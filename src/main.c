#include "mik32_hal_spi.h"
#include "mik32sd.h"
#include "mik32fs.h"
#include "mik32_hal_usart.h"
#include "mik32_stdio.h"

#include "cli.h"
/*
 * Данный пример демонстрирует работу драйвера карт SD и
 * файловой системы, совместимой с файловой системой FAT32
 */

//#define READ_EXAMPLE
#define WRITE_EXAMPLE


#define READ_BUFFER_LENGTH  20

SPI_HandleTypeDef hspi_sd;
MIK32SD_Descriptor_TypeDef sd;
MIK32FAT_Descriptor_TypeDef fs;


void SystemClock_Config(void);
static void SPI0_Init(void);
static void SD_FS_Config();

void trap_handler()
{
    printf("Podstava sluchilas\n");
}

int main()
{
    SystemClock_Config();
    
    mik32_stdio_init(UART_0, 9600);
    SPI0_Init();

    printf("\n\n*** Start ***\n");
    cli_command();


    SD_FS_Config();

    // MIK32FAT_Status_TypeDef res;

#ifdef READ_EXAMPLE
    printf("\nReading file example\n");
    FAT_File_t read_file;
    res = MIK32FAT_FileOpen(&read_file, &fs, "TESTS/READ.TXT", 'R');
    printf("TESTS/READ.TXT file open status: %d\n", res);
    if (res != FAT_OK)
    {
        while (1)
        {
            printf("Error occured with file TESTS/READ.TXT, status %u\n", res);
            HAL_DelayMs(5000);
        }
    }
    static char read_buffer[READ_BUFFER_LENGTH];
    uint8_t i = read_file.len / (READ_BUFFER_LENGTH-1);
    if (read_file.len % (READ_BUFFER_LENGTH-1) != 0) i += 1;
    uint32_t bytes_read;
    printf("Text:\n");
    while (i > 0)
    {
        bytes_read = MIK32FAT_ReadFile(&read_file, read_buffer, READ_BUFFER_LENGTH-1);
        if (bytes_read == 0)
        {
            printf("Error occured while file reading, stop.\n");
            break;
        }
        else 
        {
            /* Вставить символ возврата каретки для корректной печати */
            read_buffer[bytes_read] = '\0';
            printf("%s", read_buffer);
        }
        i -= 1;
    }
#endif
#ifdef WRITE_EXAMPLE
    // printf("\nWriting file example\n");
    // FAT_File_t write_file;
    // res = MIK32FAT_FileOpen(&write_file, &fs, "TESTS/WRITE1.TXT", 'W');
    // printf("TESTS/WRITE1.TXT file open status: %d\n", res);
    // if (res != FAT_OK)
    // {
    //     while (1)
    //     {
    //         printf("Error occured with file TESTS/WRITE1.TXT, status %u\n", res);
    //         HAL_DelayMs(5000);
    //     }
    // }
    // char str[] = "Writing string to file\n";
    // printf("Wrote bytes: %d\n", (unsigned)MIK32FAT_WriteFile(&write_file, str, strlen(str)-1));
    // printf("Close status: %d\n", (unsigned)MIK32FAT_FileClose(&write_file));
    printf("\nWriting file example\n");
    mik32fs_set_disk(&sd);
    mik32fs_set_fs(&fs);
    FILE *write_file = mik32fs_fopen("TESTS/WRITE1", 'W');
    if (write_file == NULL)
    {
        printf("Proizoshla podstava.\n");
    }
    char str[] = "Writing string to file\n";
    printf("Wrote bytes: %d\n", (unsigned)mik32_fs_write(NULL, write_file->_cookie, str, strlen(str)-1));
    printf("Close status: %d\n", (unsigned)mik32_fs_close(NULL, write_file->_cookie));
#endif
}


void SystemClock_Config(void)
{
    PCC_InitTypeDef PCC_OscInit = {0};

    PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
    PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
    PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
    PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_OSC32K;
    PCC_OscInit.AHBDivider = 0;
    PCC_OscInit.APBMDivider = 0;
    PCC_OscInit.APBPDivider = 0;
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 128;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;
    HAL_PCC_Config(&PCC_OscInit);
}


static void SD_FS_Config()
{

    /* Инициализация */
    MIK32FAT_Status_TypeDef fs_res;
    fs_res = mik32fat_init(&fs, &sd);
    printf("FS initialization: %s", fs_res==MIK32FAT_STATUS_OK ? "ok\n" : "failed, ");
    if (fs_res != MIK32FAT_STATUS_OK)
    {
        switch (fs_res)
        {
            case MIK32FAT_STATUS_DISK_ERROR: printf("disk error"); break;
            case MIK32FAT_STATUS_DISK_NOT_FORM: printf("disk not mount"); break;
            default: printf("unknown error"); break;
        }
        printf("\n");
        while(1);
    }
    // printf("FS startaddr: %u\n", fs.fs_begin);
    // printf("First FAT1 startaddr: %u\n", fs.fat1_begin);
    // printf("First FAT2 startaddr: %u\n", fs.fat2_begin);
    // printf("First data cluster: %u\n", fs.data_region_begin);
    // printf("FAT length: %u\n", fs.param.fat_length);
    // printf("Num of FATs: %u\n", fs.param.num_of_fats);
    // printf("Sectors per cluster: %u\n", fs.param.sec_per_clust);
}

static void SPI0_Init(void)
{
    hspi_sd.Instance = SPI_0;
    /* Режим SPI */
    hspi_sd.Init.SPI_Mode = HAL_SPI_MODE_MASTER;
    /* Настройки */
    hspi_sd.Init.BaudRateDiv = SPI_BAUDRATE_DIV256;
    hspi_sd.Init.CLKPhase = SPI_PHASE_ON;
    hspi_sd.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi_sd.Init.Decoder = SPI_DECODER_NONE;
    hspi_sd.Init.ThresholdTX = 1;
    hspi_sd.Init.ManualCS = SPI_MANUALCS_ON; /* Настройки ручного режима управления сигналом CS */
    hspi_sd.Init.ChipSelect = SPI_CS_0;       /* Выбор ведомого устройства в автоматическом режиме управления CS */
    if (HAL_SPI_Init(&hspi_sd) != HAL_OK)
    {
        xprintf("SPI_Init_Error\n");
    }
}