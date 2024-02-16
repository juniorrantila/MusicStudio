#include <CLI/ArgumentParser.h>
#include <Main/Main.h>
#include <Core/System.h>

#include <LittleFS/LittleFS.h>

#include <stdio.h>
#include <assert.h>

#include <string.h>

static int lfs_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int lfs_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int lfs_erase(const struct lfs_config* c, lfs_block_t block);
static int lfs_sync(const struct lfs_config*);

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    c_string project_name = nullptr;
    TRY(argument_parser.add_positional_argument("project-name"sv, [&](c_string arg) {
        project_name = arg;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    lfs_size_t block_count = 8;
    lfs_size_t block_size = 4096;
    const struct lfs_config cfg = {
        .context = calloc(block_count, block_size),

        .read  = lfs_read,
        .prog  = lfs_prog,
        .erase = lfs_erase,
        .sync  = lfs_sync,

        .read_size = 16,
        .prog_size = 16,
        .block_size = block_size,
        .block_count = block_count,
        .block_cycles = 500,
        .cache_size = 16,
        .lookahead_size = 16,
    };

    lfs_t lfs;
    lfs_format(&lfs, &cfg);
    lfs_mount(&lfs, &cfg);

    u32 read_count = 0;
    lfs_file_t file;
    lfs_file_open(&lfs, &file, "read_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_write(&lfs, &file, &read_count, sizeof(read_count));

    lfs_file_close(&lfs, &file);
    lfs_unmount(&lfs);

    FILE* f = fopen(project_name, "wb+");
    fwrite(cfg.context, 1, block_count * block_size, f);
    fclose(f);

    return 0;
}

static int lfs_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    memcpy(buffer, ((u8*)c->context) + (block * c->block_size) + off, size);
    return 0;
}

static int lfs_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	memcpy(((u8*)c->context)+ (block * c->block_size) + off, buffer, size);
    return 0;
}

static int lfs_erase(const struct lfs_config* c, lfs_block_t block)
{
    memset(((u8*)c->context) + (block * c->block_size), 0, c->block_size);
    return 0;
}

static int lfs_sync(const struct lfs_config*)
{
	return 0;
}

