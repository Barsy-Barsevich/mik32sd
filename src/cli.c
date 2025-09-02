#include "cli.h"

static MIK32SD_Descriptor_TypeDef sd;
static MIK32FAT_Descriptor_TypeDef fat;

void cli_command(void)
{
    static char buf[50];
    size_t cnt = 0;
    while(1)
    {
        printf("> ");
        char sym = '\0';
        while (sym != '\r' && sym != '\n')
        {
            sym = getchar();
            putchar(sym);
            if (sym == '\b')
            {
                if (cnt > 0) cnt--;
            }
            else if (sym != '\r' && sym != '\n')
            {
                buf[cnt++] = sym;
            }
            else
            {
                buf[cnt] = '\0';
                cnt = 0;
            }
        }

        if (strcmp(buf, "help")==0)
        {
            cli_help();
        }
        else if (strcmp(buf, "sdinit")==0)
        {
            cli_sd_init();
        }
        else if (strcmp(buf, "spiinit")==0)
        {
            cli_spi_init();
        }
        else if (strcmp(buf, "spiex")==0 || strcmp(buf, "ex")==0)
        {
            cli_spi_ex();
        }
        else if (strcmp(buf, "spiw")==0)
        {
            cli_spi_write(10);
        }
        else if (strcmp(buf, "fatinit")==0)
        {
            cli_fatinit();
        }
        else if (strcmp(buf, "sread")==0)
        {
            cli_sread();
        }
        else if (strcmp(buf, "stest")==0)
        {
            cli_stest();
        }
        else if (strcmp(buf, "csdown")==0)
        {
            cli_csdown();
        }
        else if (strcmp(buf, "csup")==0)
        {
            cli_csup();
        }
        else if (strcmp(buf, "send512")==0)
        {
            cli_send512();
        }
        else if (strcmp(buf, "read512")==0)
        {
            cli_read512();
        }
        else if (strcmp(buf, "swrite")==0)
        {
            cli_swrite();
        }
        else if (strcmp(buf, "spr")==0)
        {
            cli_fat_spr();
        }
        else if (strcmp(buf, "mkdir")==0)
        {
            cli_fat_mkdir();
        }
        else if (strcmp(buf, "touch")==0)
        {
            cli_fat_mkfile();
        }
        else if (strcmp(buf, "rm")==0)
        {
            cli_fat_rm();
        }
        else if (strcmp(buf, "fsparam")==0)
        {
            cli_fsparam();
        }
        else if (strcmp(buf, "fileopen")==0)
        {
            cli_fileopen();
        }
        else if (strcmp(buf, "fbn")==0)
        {
            cli_fat_fbn();
        }
        else if (strcmp(buf, "fbp")==0 || strcmp(buf, "cd")==0)
        {
            cli_fat_fbp();
        }
        else if (strcmp(buf, "fcbp")==0)
        {
            cli_fat_fcbp();
        }
        else if (strcmp(buf, "ls")==0)
        {
            cli_fat_ls();
        }
        else if (strcmp(buf, "pwd")==0)
        {
            cli_fat_pwd();
        }
        else if (strcmp(buf, "readbyte")==0)
        {
            cli_file_readbyte();
        }
        else if (strcmp(buf, "")==0)
        {
            //do nothing
        }
        else
        {
            printf("Incorrect command.\n");
        }
    }
}

void cli_help(void)
{
    printf("* sdinit\n");
    printf("* spiinit\n");
    printf("* spiex * ex\n");
    printf("* csdown\n");
    printf("* csup\n");
    printf("* spiw\n");
    printf("* fatinit\n");
    printf("* sread\n");
    printf("* stest\n");
    printf("* send512\n");
    printf("* read512\n");
    printf("* swrite\n");
    printf("* spr\n");
    printf("* mkdir\n");
    printf("* touch\n");
    printf("* rm");
    printf("* fsparam\n");
    printf("* fileopen\n");
    printf("* fbn\n");
    printf("* fbp * cd\n");
    printf("* fcbp\n");
    printf("* ls\n");
    printf("* pwd\n");
    printf("* readbyte\n");
}

void cli_spi_init(void)
{
    mik32_sd_spi_cfg_t cfg = {
        .host = SPI_0,
        .cs_gpio = GPIO_0,
        .cs_pin = GPIO_PIN_4,
    };
    bool res = mik32_sd_spi_init(&sd.spi, &cfg);
    printf("status: %s\n", res==true ? "OK" : "Error");
}

void cli_sd_init(void)
{
    MIK32SD_Status_TypeDef res;
    MIK32SD_Config_TypeDef sd_cfg = {
        .spi.host = SPI_0,
        .spi.cs_gpio = GPIO_0,
        .spi.cs_pin = GPIO_PIN_4,
        .spi.frequency = 1000000,
    };
    res = mik32_sd_init(&sd, &sd_cfg);
    printf("SD init: ");
    mik32sd_diag_decode_status(res);
    printf("Type: ");
    mik32sd_diag_decode_sd_type(sd.type);
}

void cli_spi_ex(void)
{
    int inval = 0;
    char str[20];
    char sym = '\0';
    int cnt = 0;
    printf("Enter sector number: > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            str[cnt++] = sym;
        }
        else
        {
            str[cnt] = '\0';
        }
    }
    int m = str[0] - 0x30;
    if (m > 9) m -= 7;
    int l = str[1] - 0x30;
    if (l > 9) l -= 7;
    inval = l + 16*m;

    uint8_t res = mik32_sd_spi_ex(&sd.spi, inval);
    printf("res: 0x%02X\n", res);
}

void cli_csdown(void)
{
    mik32_sd_spi_cs_down(&sd.spi);
}

void cli_csup(void)
{
    mik32_sd_spi_cs_up(&sd.spi);
}

void cli_spi_write(int v)
{
    char d[] = {'A','B','C'};
    dma_status_t res;
    while(1)
    {
        res = mik32_sd_spi_sector_write(&sd.spi, d, 5);
        printf("status: %u\n", res);
        printf("0x%02X 0x%02X 0x%02X\n", d[0], d[1], d[2]);
    }
}

void cli_fatinit(void)
{
    MIK32FAT_Status_TypeDef res;
    res = mik32fat_init(&fat, &sd);
    mik32fat_decode_status(res);
}

void cli_sread()
{
    int sec = 0;
    char str[20];
    char sym = '\0';
    int cnt = 0;
    printf("Enter sector number: > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            str[cnt++] = sym;
        }
        else
        {
            str[cnt] = '\0';
        }
    }
    sec = atoi(str);
    mik32sd_command_sector_read(&sd, sec, fat.buffer);
    //512
    for (int i=0; i<512; i+=16)
    {
        for (int j=0; j<16; j++)
        {
            printf("%02X ", fat.buffer[i+j]);
        }
        printf("    ");
        for (int j=0; j<16; j++)
        {
            char s = fat.buffer[i+j];
            if ( (s >= '0' && s <= '9') || (s >= 'A' && s <= 'Z') || (s >= 'a' && s <= 'z'))
            {
                printf("%c", s);
            }
            else printf(".");
        }
        printf("\n");
    }
}

static char reserved_buff[512];

void cli_stest()
{
    mik32sd_command_sector_read(&sd, 1000, fat.buffer);
    //512
    for (int i=0; i<512; i+=16)
    {
        for (int j=0; j<16; j++)
        {
            printf("%02X ", fat.buffer[i+j]);
        }
        printf("\n");
    }
    mik32sd_command_sector_erase(&sd, 1000);
    mik32sd_command_sector_read(&sd, 1000, reserved_buff);
    for (int i=0; i<512; i+=16)
    {
        for (int j=0; j<16; j++)
        {
            printf("%02X ", reserved_buff[i+j]);
        }
        printf("\n");
    }
    mik32sd_command_sector_write(&sd, 1000, fat.buffer);
    // mik32sd_command_sector_read(&sd, 1000, reserved_buff);
    // for (int i=0; i<512; i+=16)
    // {
    //     for (int j=0; j<16; j++)
    //     {
    //         printf("%02X ", reserved_buff[i+j]);
    //     }
    //     printf("\n");
    // }
}

void cli_send512(void)
{
    for (int i=0; i<500; i++)
    {
        mik32_sd_spi_ex(&sd.spi, 0x53);
    }
}

void cli_read512(void)
{
    for (int i=0; i<512; i++)
    {
        mik32_sd_spi_ex(&sd.spi, 0xFF);
    }
}

void cli_swrite(void)
{
    mik32sd_command_sector_write(&sd, 0, fat.buffer);
}

void cli_fat_spr(void)
{
    mik32fat_set_pointer_to_root(&fat);
}

void cli_fat_mkdir(void)
{
    char sym = '\0';
    int cnt = 0;
    printf("Enter directory name (path): > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            reserved_buff[cnt++] = sym;
        }
        else
        {
            reserved_buff[cnt] = '\0';
        }
    }
    mik32fat_decode_status(mik32fat_create(&fat, reserved_buff, true));
}

void cli_fat_mkfile(void)
{
    char sym = '\0';
    int cnt = 0;
    printf("Enter file name (path): > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            reserved_buff[cnt++] = sym;
        }
        else
        {
            reserved_buff[cnt] = '\0';
        }
    }
    mik32fat_decode_status(mik32fat_create(&fat, reserved_buff, false));
}

void cli_fat_rm(void)
{
    char sym = '\0';
    int cnt = 0;
    printf("Enter file name (path): > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            reserved_buff[cnt++] = sym;
        }
        else
        {
            reserved_buff[cnt] = '\0';
        }
    }
    mik32fat_decode_status(mik32fat_delete(&fat, reserved_buff));
}

void cli_fsparam(void)
{
    mik32fat_diag_fat_info(&fat);
}

MIK32FAT_File_TypeDef file;
void cli_fileopen(void)
{
    char sym = '\0';
    int cnt = 0;
    printf("Enter file name (path): > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            reserved_buff[cnt++] = sym;
        }
        else
        {
            reserved_buff[cnt] = '\0';
        }
    }
    sym = '\0';
    cnt = 0;
    printf("Enter mode: > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            reserved_buff[30+cnt++] = sym;
        }
        else
        {
            reserved_buff[30+cnt] = '\0';
        }
    }
    printf("Name: *%s*; mode: *%s*\n", reserved_buff, reserved_buff+30);
    mik32fat_decode_status(mik32fat_file_open(&file, &fat, reserved_buff, reserved_buff+30));
    mik32fat_diag_decode_file(&file);
}


void cli_fat_fbn(void)
{
    char sym = '\0';
    int cnt = 0;
    printf("Enter name: > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            reserved_buff[cnt++] = sym;
        }
        else
        {
            reserved_buff[cnt] = '\0';
        }
    }
    mik32fat_decode_status(mik32fat_find_by_name(&fat, reserved_buff));
    printf("Temp cluster: %u\n", (unsigned)fat.temp.cluster);
}

void cli_fat_fbp(void)
{
    char sym = '\0';
    int cnt = 0;
    printf("Enter name (path): > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            reserved_buff[cnt++] = sym;
        }
        else
        {
            reserved_buff[cnt] = '\0';
        }
    }
    mik32fat_decode_status(mik32fat_find_by_path(&fat, reserved_buff));
    printf("Temp cluster: %u\n", (unsigned)fat.temp.cluster);
}

void cli_fat_fcbp(void)
{
    char sym = '\0';
    int cnt = 0;
    printf("Enter name (path): > ");
    while (sym != '\n' && sym != '\r')
    {
        sym = getchar();
        putchar(sym);
        if (sym == '\b')
        {
            if (cnt > 0) cnt--;
        }
        else if (sym != '\n' && sym != '\r')
        {
            reserved_buff[cnt++] = sym;
        }
        else
        {
            reserved_buff[cnt] = '\0';
        }
    }
    mik32fat_decode_status(mik32fat_find_or_create_by_path(&fat, reserved_buff, true));
    printf("Temp cluster: %u\n", (unsigned)fat.temp.cluster);
}

void cli_fat_ls(void)
{
    MIK32FAT_Status_TypeDef res = mik32fat_utils_ls(&fat, stdout);
    if (res != MIK32FAT_STATUS_OK)
    {
        printf("mik32fat_ls: \n");
        mik32fat_decode_status(res);
    }
}

void cli_fat_pwd(void)
{
    printf("%s\n", fat.temp.name);
}

void cli_file_readbyte(void)
{
    char dummy = 'X';
    printf("Status: %u\n", mik32fat_file_read_byte(&file, &dummy));
    printf("Res: %c\n", dummy);
}