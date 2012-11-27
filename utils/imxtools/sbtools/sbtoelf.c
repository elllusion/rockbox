/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Bertrik Sikken
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * .sb file parser and chunk extractor
 *
 * Based on amsinfo, which is
 * Copyright © 2008 Rafaël Carré <rafael.carre@gmail.com>
 */

#define _ISOC99_SOURCE /* snprintf() */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <strings.h>
#include <getopt.h>

#include "crypto.h"
#include "elf.h"
#include "sb.h"
#include "sb1.h"
#include "misc.h"

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* If you find a firmware that breaks the known format ^^ */
#define assert(a) do { if(!(a)) { fprintf(stderr,"Assertion \"%s\" failed in %s() line %d!\n\nPlease send us your firmware!\n",#a,__func__,__LINE__); exit(1); } } while(0)

#define crypto_cbc(...) \
    do { int ret = crypto_cbc(__VA_ARGS__); \
        if(ret != CRYPTO_ERROR_SUCCESS) \
            bug("crypto_cbc error: %d\n", ret); \
    }while(0)

/* globals */

static char *g_out_prefix;
static bool g_elf_simplify = true;

static void elf_printf(void *user, bool error, const char *fmt, ...)
{
    if(!g_debug && !error)
        return;
    (void) user;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

static void elf_write(void *user, uint32_t addr, const void *buf, size_t count)
{
    FILE *f = user;
    fseek(f, addr, SEEK_SET);
    fwrite(buf, count, 1, f);
}

static void extract_elf_section(struct elf_params_t *elf, int count, uint32_t id)
{
    char name[5];
    char *filename = xmalloc(strlen(g_out_prefix) + 32);
    sb_fill_section_name(name, id);
    sprintf(filename, "%s%s.%d.elf", g_out_prefix, name, count);
    if(g_debug)
        printf("Write boot section %s to %s\n", name, filename);
    
    FILE *fd = fopen(filename, "wb");
    free(filename);
    
    if(fd == NULL)
        return;
    if(g_elf_simplify)
        elf_simplify(elf);
    elf_write_file(elf, elf_write, elf_printf, fd);
    fclose(fd);
}

static void extract_sb_section(struct sb_section_t *sec)
{
    if(sec->is_data)
    {
        char sec_name[5];
        char *filename = xmalloc(strlen(g_out_prefix) + 32);
        sb_fill_section_name(sec_name, sec->identifier);
        sprintf(filename, "%s%s.bin", g_out_prefix, sec_name);
        FILE *fd = fopen(filename, "wb");
        if(fd == NULL)
            bugp("Cannot open %s for writing\n", filename);
        if(g_debug)
            printf("Write data section %s to %s\n", sec_name, filename);
        free(filename);
        
        for(int j = 0; j < sec->nr_insts; j++)
        {
            assert(sec->insts[j].inst == SB_INST_DATA);
            fwrite(sec->insts[j].data, sec->insts[j].size, 1, fd);
        }
        fclose(fd);
    }

    int elf_count = 0;
    struct elf_params_t elf;
    elf_init(&elf);

    for(int i = 0; i < sec->nr_insts; i++)
    {
        struct sb_inst_t *inst = &sec->insts[i];
        switch(inst->inst)
        {
            case SB_INST_LOAD:
                elf_add_load_section(&elf, inst->addr, inst->size, inst->data);
                break;
            case SB_INST_FILL:
                elf_add_fill_section(&elf, inst->addr, inst->size, inst->pattern);
                break;
            case SB_INST_CALL:
            case SB_INST_JUMP:
                elf_set_start_addr(&elf, inst->addr);
                extract_elf_section(&elf, elf_count++, sec->identifier);
                elf_release(&elf);
                elf_init(&elf);
                break;
            default:
                /* ignore mode and nop */
                break;
        }
    }

    if(!elf_is_empty(&elf))
        extract_elf_section(&elf, elf_count, sec->identifier);
    elf_release(&elf);
}

static void extract_sb_file(struct sb_file_t *file)
{
    for(int i = 0; i < file->nr_sections; i++)
        extract_sb_section(&file->sections[i]);
}

static void extract_elf(struct elf_params_t *elf, int count)
{
    char *filename = xmalloc(strlen(g_out_prefix) + 32);
    sprintf(filename, "%s.%d.elf", g_out_prefix, count);
    if(g_debug)
        printf("Write boot content to %s\n", filename);

    FILE *fd = fopen(filename, "wb");
    free(filename);

    if(fd == NULL)
        return;
    if(g_elf_simplify)
        elf_simplify(elf);
    elf_write_file(elf, elf_write, elf_printf, fd);
    fclose(fd);
}

static void extract_sb1_file(struct sb1_file_t *file)
{
    int elf_count = 0;
    struct elf_params_t elf;
    elf_init(&elf);

    for(int i = 0; i < file->nr_insts; i++)
    {
        struct sb1_inst_t *inst = &file->insts[i];
        switch(inst->cmd)
        {
            case SB1_INST_LOAD:
                elf_add_load_section(&elf, inst->addr, inst->size, inst->data);
                break;
            case SB1_INST_FILL:
                elf_add_fill_section(&elf, inst->addr, inst->size, inst->pattern);
                break;
            case SB1_INST_CALL:
            case SB1_INST_JUMP:
                elf_set_start_addr(&elf, inst->addr);
                extract_elf(&elf, elf_count++);
                elf_release(&elf);
                elf_init(&elf);
                break;
            default:
                /* ignore mode and nop */
                break;
        }
    }

    if(!elf_is_empty(&elf))
        extract_elf(&elf, elf_count);
    elf_release(&elf);
}

static void usage(void)
{
    printf("Usage: sbtoelf [options] sb-file\n");
    printf("Options:\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -o <prefix>\tEnable output and set prefix\n");
    printf("  -d/--debug\tEnable debug output*\n");
    printf("  -k <file>\tAdd key file\n");
    printf("  -z\t\tAdd zero key\n");
    printf("  -r\t\tUse raw command mode\n");
    printf("  -a/--add-key <key>\tAdd single key (hex or usbotp)\n");
    printf("  -n/--no-color\tDisable output colors\n");
    printf("  -l/--loopback <file>\tProduce sb file out of extracted description*\n");
    printf("  -f/--force\tForce reading even without a key*\n");
    printf("  -1/--v1\tForce to read file as a version 1 file\n");
    printf("  -2/--v2\tForce to read file as a version 2 file\n");
    printf("  -s/--no-simpl\tPrevent elf files from being simplified*\n");
    printf("  -x\t\tUse default sb1 key\n");
    printf("Options marked with a * are for debug purpose only\n");
    exit(1);
}

static void sb_printf(void *user, bool error, color_t c, const char *fmt, ...)
{
    (void) user;
    (void) error;
    va_list args;
    va_start(args, fmt);
    color(c);
    vprintf(fmt, args);
    va_end(args);
}

static struct crypto_key_t g_zero_key =
{
    .method = CRYPTO_KEY,
    .u.key = {0}
};

static struct crypto_key_t g_default_xor_key =
{
    .method = CRYPTO_XOR_KEY,
    .u.xor_key =
    {
        {.k = {0x67ECAEF6, 0xB31FB961, 0x118A9F4C, 0xA32A97DA,
        0x6CC39617, 0x5BC00314, 0x9D430685, 0x4D7DB502,
        0xA347685E, 0x3C87E86C, 0x8987AAA0, 0x24B78EF1,
        0x893B9605, 0x9BB8C2BE, 0x6D9544E2, 0x375B525C}},
        {.k = {0x3F424704, 0x53B5A331, 0x6AD345A5, 0x20DCEC51,
        0x743C8D3B, 0x444B3792, 0x0AF429569, 0xB7EE1111,
        0x583BF768, 0x9683BF9A, 0x0B032D799, 0xFE4E78ED,
        0xF20D08C2, 0xFA0BE4A2, 0x4D89C317, 0x887B2D6F}}
    }
};

enum sb_version_guess_t
{
    SB_VERSION_1,
    SB_VERSION_2,
    SB_VERSION_UNK,
};

enum sb_version_guess_t guess_sb_version(const char *filename)
{
#define ret(x) do { fclose(f); return x; } while(0)
    FILE *f = fopen(filename, "rb");
    if(f == NULL)
        bugp("Cannot open file for reading\n");
    // check signature
    uint8_t sig[4];
    if(fseek(f, 20, SEEK_SET))
        ret(SB_VERSION_UNK);
    if(fread(sig, 4, 1, f) != 1)
        ret(SB_VERSION_UNK);
    if(memcmp(sig, "STMP", 4) != 0)
        ret(SB_VERSION_UNK);
    // check header size (v1)
    uint32_t hdr_size;
    if(fseek(f, 8, SEEK_SET))
        ret(SB_VERSION_UNK);
    if(fread(&hdr_size, 4, 1, f) != 1)
        ret(SB_VERSION_UNK);
    if(hdr_size == 0x34)
        ret(SB_VERSION_1);
    // check header size (v2)
    if(fseek(f, 32, SEEK_SET))
        ret(SB_VERSION_UNK);
    if(fread(&hdr_size, 4, 1, f) != 1)
        ret(SB_VERSION_UNK);
    if(hdr_size == 0xc)
        ret(SB_VERSION_2);
    ret(SB_VERSION_UNK);
#undef ret
}

int main(int argc, char **argv)
{
    bool raw_mode = false;
    const char *loopback = NULL;
    bool force_sb1 = false;
    bool force_sb2 = false;

    /* decrypt the xor key which is xor'ed */
    for(int i = 0; i < 2; i++)
        for(int j = 0; j < 16; j++)
            g_default_xor_key.u.xor_key[i].k[j] ^= 0xaa55aa55;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"add-key", required_argument, 0, 'a'},
            {"no-color", no_argument, 0, 'n'},
            {"loopback", required_argument, 0, 'l'},
            {"force", no_argument, 0, 'f'},
            {"v1", no_argument, 0, '1'},
            {"v2", no_argument, 0, '2'},
            {"no-simpl", no_argument, 0, 's'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?do:k:zra:nl:f12xs", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'l':
                if(loopback)
                    bug("Only one loopback file can be specified !\n");
                loopback = optarg;
                break;
            case 'n':
                enable_color(false);
                break;
            case 'd':
                g_debug = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'f':
                g_force = true;
                break;
            case 'k':
            {
                if(!add_keys_from_file(optarg))
                    bug("Cannot add keys from %s\n", optarg);
                break;
            }
            case 'z':
                add_keys(&g_zero_key, 1);
                break;
            case 'x':
                add_keys(&g_default_xor_key, 1);
                break;
            case 'r':
                raw_mode = true;
                break;
            case 'a':
            {
                struct crypto_key_t key;
                char *s = optarg;
                if(!parse_key(&s, &key))
                    bug("Invalid key specified as argument\n");
                if(*s != 0)
                    bug("Trailing characters after key specified as argument\n");
                add_keys(&key, 1);
                break;
            }
            case '1':
                force_sb1 = true;
                break;
            case '2':
                force_sb2 = true;
                break;
            case 's':
                g_elf_simplify = false;
                break;
            default:
                abort();
        }
    }

    if(force_sb1 && force_sb2)
        bug("You cannot force both version 1 and 2\n");

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    const char *sb_filename = argv[optind];

    enum sb_version_guess_t ver = guess_sb_version(sb_filename);

    if(force_sb2 || ver == SB_VERSION_2)
    {
        enum sb_error_t err;
        struct sb_file_t *file = sb_read_file(sb_filename, raw_mode, NULL, sb_printf, &err);
        if(file == NULL)
        {
            color(OFF);
            printf("SB read failed: %d\n", err);
            return 1;
        }

        color(OFF);
        if(g_out_prefix)
            extract_sb_file(file);
        if(g_debug)
        {
            color(GREY);
            printf("[Debug output]\n");
            sb_dump(file, NULL, sb_printf);
        }
        if(loopback)
        {
            /* sb_read_file will fill real key and IV but we don't want to override
            * them when looping back otherwise the output will be inconsistent and
            * garbage */
            file->override_real_key = false;
            file->override_crypto_iv = false;
            sb_write_file(file, loopback);
        }
        sb_free(file);
    }
    else if(force_sb1 || ver == SB_VERSION_1)
    {
        enum sb1_error_t err;
        struct sb1_file_t *file = sb1_read_file(sb_filename, NULL, sb_printf, &err);
        if(file == NULL)
        {
            color(OFF);
            printf("SB read failed: %d\n", err);
            return 1;
        }

        color(OFF);
        if(g_out_prefix)
            extract_sb1_file(file);
        if(g_debug)
        {
            color(GREY);
            printf("[Debug output]\n");
            sb1_dump(file, NULL, sb_printf);
        }
        if(loopback)
            sb1_write_file(file, loopback);
        
        sb1_free(file);
    }
    else
    {
        color(OFF);
        printf("Cannot guess file type, are you sure it's a valid image ?\n");
        return 1;
    }
    clear_keys();
    
    return 0;
}
