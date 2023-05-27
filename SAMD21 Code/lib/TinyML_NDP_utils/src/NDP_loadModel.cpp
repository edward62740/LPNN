/*
** Copyright (c) 2021, Syntiant
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/
//Line 47/48 are commented out to avoid contention
#include "NDP_loadModel.h"

SdFat SD;

// SdCardFactory constructs and initializes the appropriate card.
SdCardFactory cardFactory;
// Pointer to generic SD card.
SdCard *mCard = nullptr;
uint32_t const ERASE_SIZE = 262144L;

uint32_t cardSectorCount = 0;
uint8_t sectorBuffer[512];

byte runningFromFlash = 0; // indicates successful boot from flash
byte patchApplied = 0;     // Patch applied (from slot 1) to uilib load (slot 0)

int loadModel(String model)
{
    File myFile;
    uint32_t numBytes;
    uint8_t fileBuf[1024] __attribute__((aligned(4)));
    int s;
    byte cardInserted;
    byte FoundSerialFlash = 0;

   // delay(2000);

    // Initialize SD insertion sense pin
    pinMode(SD_CARD_SENSE, INPUT_PULLUP);

    //pinMode(ENABLE_PDM, OUTPUT);
    //digitalWrite(ENABLE_PDM, LOW); // Enable PDM clock

    // Bluebank board uses SST25VF016B. SD_CARD_SENSE pin is HIGH with card inserted
    // Tesolve board uses MX25R6435FSN. SD_CARD_SENSE pin is LOW with card inserted
    cardInserted = digitalRead(SD_CARD_SENSE);
    if (FlashType[0] == MX25R6435FSN)
    {
        cardInserted = !cardInserted;
    }

    if (!cardInserted)
    {
        // See if Serial Flash installed
        if (SerialFlash.begin(FLASH_CS))
        {
            // enable writes to flash
            // mpw. Erase device prior to testing
            digitalWrite(FLASH_CS, LOW);
            SPI1.transfer(FLASH_EWSR); // enable write status register
            digitalWrite(FLASH_CS, HIGH);
            delayMicroseconds(1);

            digitalWrite(FLASH_CS, LOW);
            SPI1.transfer(FLASH_WRSR); // write status register
            SPI1.transfer(0);          // clear write protect bits
            digitalWrite(FLASH_CS, HIGH);
            delayMicroseconds(1);

            digitalWrite(FLASH_CS, LOW);
            SPI1.transfer(FLASH_WREN); // write enable command
            digitalWrite(FLASH_CS, HIGH);
            delayMicroseconds(1);

            byte found = flashCat(model); // read filenames in Serial Flash looking for bin file "model"
            if (found)
            {
                SerialFlashFile file = SerialFlash.open(model.c_str());
                unsigned long count = file.size();
                unsigned long n = count;

                while (file.available())
                {
                    unsigned long rd = n;
                    if (rd > sizeof(fileBuf))
                        rd = sizeof(fileBuf);
                    numBytes = file.read(fileBuf, rd);

                    s = NDP.loadLog(fileBuf, numBytes);
                    if (s != SYNTIANT_NDP_ERROR_NONE && s != SYNTIANT_NDP_ERROR_MORE)
                    {
                        return ERROR_LOADING_FLASH;
                    }
                    n = n - rd;
                }
            }
            FoundSerialFlash = 1;
            if (s == SYNTIANT_NDP_ERROR_NONE)
            {
                return LOADED_FROM_SERIAL_FLASH;
            }
        }
        return NO_SD;
    }

    if (!SD.begin(SDCARD_SS_PIN))
    {
        return SD_NOT_INITIALIZED;
    }

    // open the file. Have to close this one before opening another.
    myFile = SD.open(model, FILE_READ);
    if (!myFile)
    {
        myFile.close();
        return BIN_NOT_OPENED;
    }

    // read from the SD card file until there's nothing else in it:
    while (myFile.available())
    {
        numBytes = myFile.read(fileBuf, sizeof(fileBuf));
        s = NDP.loadLog(fileBuf, numBytes);
        if (s != SYNTIANT_NDP_ERROR_NONE && s != SYNTIANT_NDP_ERROR_MORE)
        {
            myFile.close();
            return ERROR_LOADING_SD;
        }
    }

    if (s == SYNTIANT_NDP_ERROR_NONE)
    {
        myFile.close();
        return BIN_LOAD_OK;
    }
}

// Clears the onboard SST25VF016B device, & copies the SD flash files to it
void copySdToFlash()
{


    if (!SD.begin(SD_CS))
    {
        Serial.println("Unable to access SD card");
    }
    if (!SerialFlash.begin(FLASH_CS))
    {
        Serial.println("Unable to access SPI Flash chip");
    }
    Serial.println("SD card initialized OK");

    // enable writes to flash
    digitalWrite(FLASH_CS, LOW);
    SPI1.transfer(0x50); // enable write status register
    digitalWrite(FLASH_CS, HIGH);
    delayMicroseconds(1);

    digitalWrite(FLASH_CS, LOW);
    SPI1.transfer(0x01); // write status register
    SPI1.transfer(0);    // clear write protect bits
    digitalWrite(FLASH_CS, HIGH);
    delayMicroseconds(1);

    digitalWrite(FLASH_CS, LOW);
    SPI1.transfer(0x06); // write enable command
    digitalWrite(FLASH_CS, HIGH);
    delayMicroseconds(1);

    digitalWrite(FLASH_CS, LOW);
    SPI1.transfer(0x05);
    byte status = SPI1.transfer(0);
    digitalWrite(FLASH_CS, HIGH);
    Serial.print("Status Register before erase = 0x");
    Serial.println(status, HEX);

    // erase Serial flash
    Serial.println("Erasing Chip");
    digitalWrite(LED_BUILTIN, HIGH);
    SerialFlash.eraseAll();
    SerialFlash.wait();
    Serial.println("Done erasing");

    File rootdir = SD.open("/");

    // Copy all files from SD card to Serial Flash
    Serial.println("Copying Files From SD Card");

    while (1)
    {
        // files from the SD card
        File f = rootdir.openNextFile();
        if (!f)
            break;

        char filename[64];
        byte len = sizeof(filename);
        f.getName(filename, len);

        Serial.print(filename);
        Serial.print("    ");
        unsigned long length = f.size();
        Serial.println(length);

        // check if this file is already on the Flash chip
        if (SerialFlash.exists(filename))
        {
            Serial.println("  already exists on the Flash chip");
            SerialFlashFile ff = SerialFlash.open(filename);
            if (ff && ff.size() == f.size())
            {
                Serial.println("  size is the same, comparing data...");
                if (compareFiles(f, ff) == true)
                {
                    Serial.println("  files are identical :)");
                    f.close();
                    ff.close();
                    continue; // advance to next file
                }
                else
                {
                    Serial.println("  files are different");
                }
            }
            else
            {
                Serial.print("  size is different, ");
                Serial.print(ff.size());
                Serial.println(" bytes");
            }
            // delete the copy on the Flash chip, if different
            Serial.println("  delete file from Flash chip");
            SerialFlash.remove(filename);
        }

        // create the file on the Flash chip and copy data
        if (SerialFlash.create(filename, length))
        {
            SerialFlashFile ff = SerialFlash.open(filename);
            if (ff)
            {
                Serial.print("  copying");
                // copy data loop
                unsigned long count = 0;
                unsigned char dotcount = 9;

                unsigned long usbegin = micros();

                while (count < length)
                {
                    char buf[256];
                    unsigned int n;
                    n = f.read(buf, 256);
                    ff.write(buf, n);
                    count = count + n;
                    Serial.print(".");
                    if (++dotcount > 100)
                    {
                        Serial.println();
                        dotcount = 0;
                    }
                }
                uint32_t filesize = ff.size();
                unsigned long usend = micros();
                Serial.println();
                Serial.print("Written in ");
                Serial.print(usend - usbegin);
                Serial.print(" us, speed = ");
                Serial.print((float)filesize * 1000.0 / (float)(usend - usbegin));
                Serial.println(" kbytes/sec");

                ff.close();
                if (dotcount > 0)
                    Serial.println();
            }
            else
            {
                Serial.println("  error opening freshly created file!");
            }
        }
        else
        {
            Serial.println("  unable to create file");
        }
        f.close();
    }
    rootdir.close();
    delay(10);
    Serial.println("Finished Writing All Files");
    Serial.println("");
    Serial.println("Catalog of Serial Flash");
    SerialFlash.opendir();
    while (1)
    {
        char filename[64];
        uint32_t filesize;

        String FoundFile = "google10.bin";

        if (SerialFlash.readdir(filename, sizeof(filename), filesize))
        {
            Serial.print("  ");
            Serial.print(filename);
            Serial.print("  ");
            Serial.print(filesize);
            Serial.print(" bytes");
            Serial.println();
            if (!strcmp(FoundFile.c_str(), (char *)filename))
            {
                Serial.println("Found BIN file!!");
            }
        }
        else
        {
            break; // no more files
        }
    }

    // Read Performance Test
    Serial.println("");
    Serial.println("Read Performance Test....");
    SerialFlash.opendir();
    int filecount = 0;
    while (1)
    {
        char filename[64];
        uint32_t filesize;

        if (SerialFlash.readdir(filename, sizeof(filename), filesize))
        {
            Serial.print("  ");
            Serial.print(filename);
            Serial.print(", ");
            Serial.print(filesize);
            Serial.print(" bytes");
            SerialFlashFile file = SerialFlash.open(filename);
            if (file)
            {
                unsigned long usbegin = micros();
                unsigned long n = filesize;
                char buffer[256];
                while (n > 0)
                {
                    unsigned long rd = n;
                    if (rd > sizeof(buffer))
                        rd = sizeof(buffer);
                    file.read(buffer, rd);
                    n = n - rd;
                }
                unsigned long usend = micros();
                Serial.print(", read in ");
                Serial.print(usend - usbegin);
                Serial.print(" us, speed = ");
                Serial.print((float)filesize * 1000.0 / (float)(usend - usbegin));
                Serial.println(" kbytes/sec");
                file.close();
            }
            else
            {
                Serial.println(" error reading this file!");
            }
            filecount = filecount + 1;
        }
        else
        {
            if (filecount == 0)
            {
                Serial.println("No files found in SerialFlash memory.");
            }
            digitalWrite(LED_BUILTIN, LOW);
            break; // no more files
        }
    }
}

bool compareFiles(File &file, SerialFlashFile &ffile)
{
    file.seek(0);
    ffile.seek(0);
    unsigned long count = file.size();
    while (count > 0)
    {
        char buf1[128], buf2[128];
        unsigned long n = count;
        if (n > 128)
            n = 128;
        file.read(buf1, n);
        ffile.read(buf2, n);
        if (memcmp(buf1, buf2, n) != 0)
            return false; // differ
        count = count - n;
    }
    return true; // all data identical
}

void fatFormatSd()
{
    Serial.println("Formatting SD card");

    // Select and initialize proper card driver.
    mCard = cardFactory.newCard(SD_CONFIG);
    if (!mCard || mCard->errorCode())
    {
        Serial.println("Card init failed.");
        return;
    }

    cardSectorCount = mCard->sectorCount();
    if (!cardSectorCount)
    {
        Serial.println("Get sector count failed.");
        return;
    }

    Serial.print("Card size: ");
    Serial.print(cardSectorCount * 5.12e-7);
    Serial.println(" GB (GB = 1E9 bytes)");
    Serial.print("Card size: ");
    Serial.print(cardSectorCount / 2097152.0);
    Serial.println(" GiB (GiB = 2^30 bytes)");
    Serial.print("Card will be formatted with ");
    if (cardSectorCount > 67108864)
    {
        Serial.println("exFAT");
    }
    else if (cardSectorCount > 4194304)
    {
        Serial.println("FAT32");
    }
    else
    {
        Serial.println("FAT16");
    }
    formatCard();
}

void formatCard()
{
    ExFatFormatter exFatFormatter;
    FatFormatter fatFormatter;

    // Format exFAT if larger than 32GB.
    bool rtn = cardSectorCount > 67108864 ? exFatFormatter.format(mCard, sectorBuffer, &Serial) : fatFormatter.format(mCard, sectorBuffer, &Serial);

    if (!rtn)
    {
        Serial.println("Formatting Error");
    }
}

void eraseSd()
{
    Serial.println("Erasing SD card");

    // Select and initialize proper card driver.
    mCard = cardFactory.newCard(SD_CONFIG);
    if (!mCard || mCard->errorCode())
    {
        Serial.println("Card init failed.");
        return;
    }

    cardSectorCount = mCard->sectorCount();
    if (!cardSectorCount)
    {
        Serial.println("Get sector count failed.");
        return;
    }

    Serial.print("Card size: ");
    Serial.print(cardSectorCount * 5.12e-7);
    Serial.println(" GB (GB = 1E9 bytes)");
    Serial.print("Card size: ");
    Serial.print(cardSectorCount / 2097152.0);
    Serial.println(" GiB (GiB = 2^30 bytes)");
    eraseCard();
    Serial.println("Done!");
}

void eraseCard()
{
    Serial.println("Erasing!");
    uint32_t firstBlock = 0;
    uint32_t lastBlock;
    uint16_t n = 0;
    do
    {
        lastBlock = firstBlock + ERASE_SIZE - 1;
        if (lastBlock >= cardSectorCount)
        {
            lastBlock = cardSectorCount - 1;
        }
        if (!mCard->erase(firstBlock, lastBlock))
        {
            Serial.println("Erase failed");
        }
        Serial.print(".");
        if ((n++) % 64 == 63)
        {
            Serial.println("");
        }
        firstBlock += ERASE_SIZE;
    } while (firstBlock < cardSectorCount);

    Serial.println("");

    if (!mCard->readSector(0, sectorBuffer))
    {
        Serial.println("Error - readBlock");
    }

    Serial.println("Erase done");
}

// read filenames in Serial Flash looking for file
byte flashCat(String foundFile)
{
    byte found = 0;
    SerialFlash.opendir();

    while (1)
    {
        char filename[64];
        uint32_t filesize;
        char file_on_disk[] = "rnd_name.txt";

        if (SerialFlash.readdir(filename, sizeof(filename), filesize))
        {
            if (!strcmp(foundFile.c_str(), (char *)filename))
            {
                found = 1;
            }
        }
        else
        {
            break; // no more files
        }
    }
    return found;
}

// Read Flash "Magic Number" & File Length from the Flash File Index.
// These are held at flash address 0xffff8 (file length) & 0xffffc
// (Magic Number).  There are 8 locations where data can be held in flash.
// Each is 0x100000 in length
// If the location is used, there is a magic number, length & checksum
// at the end of the segment.
// location 0 is used for boot.
// The 4 byte magic number is at 0xffffc
// The 4 byte file length is at location 0xffff8
// The 4 byte checksum is at 0xffff4
void loadUilibFlash(byte record)
{
    int s;

    // Enable SPI Master Mode for Flash access
    enableMasterSpi();
    uint32_t flashOffset = record << 20;

    changeMasterSpiMode(MSPI_ENABLE);
    indirectWrite(CHIP_CONFIG_SPITX, FLASH_READ + flashOffset + FLASH_INDEX_LOCATION);
    writeNumBytes(0x3); // 3 + 1 == 4 bytes at a time
    changeMasterSpiMode(MSPI_TRANSFER);
    spiWait();
    changeMasterSpiMode(MSPI_UPDATE);
    changeMasterSpiMode(MSPI_TRANSFER);
    unsigned long fileLength = indirectRead(CHIP_CONFIG_SPIRX);
    Serial.println(fileLength, HEX);
    changeMasterSpiMode(MSPI_UPDATE);
    changeMasterSpiMode(MSPI_TRANSFER);
    unsigned long magicNumber = indirectRead(CHIP_CONFIG_SPIRX);

    byte flashBug = 0;

    // See if Flash in "Flash Bug" (3 byte encoded) format
    if (magicNumber == ((MAGIC_NUMBER << 8) & 0xffffffff))
    {
        flashBug = 1;
        magicNumber = MAGIC_NUMBER;
        fileLength = fileLength >> 8;
    }
    Serial.println(magicNumber, HEX);
    Serial.println(MAGIC_NUMBER, HEX);
    // Check if magic number is correct
    if (magicNumber == MAGIC_NUMBER)
    {
        changeMasterSpiMode(MSPI_IDLE);

        // start to read uilib from flash address 0x000000
        int bufferCount = 0;
        long bufferIndex = 0;

        for (long i = 0; i < (int)fileLength; i += 4)
        {
            if (bufferIndex == 0)
            {
                changeMasterSpiMode(MSPI_IDLE);
                changeMasterSpiMode(MSPI_ENABLE);
                indirectWrite(CHIP_CONFIG_SPITX, FLASH_READ + flashOffset + i);
                writeNumBytes(0x3); // read 3 + 1 = 4 bytes
                changeMasterSpiMode(MSPI_TRANSFER);
                spiWait();
            }

            changeMasterSpiMode(MSPI_UPDATE);
            changeMasterSpiMode(MSPI_TRANSFER);
            long read = indirectRead(CHIP_CONFIG_SPIRX);
            if (flashBug)
            {
                ilibBuf[bufferIndex + 0] = (read >> 8) & 0xff;
                ilibBuf[bufferIndex + 1] = (read >> 16) & 0xff;
                ilibBuf[bufferIndex + 2] = (read >> 24) & 0xff;
                bufferIndex += 3;
            }
            else
            {
                ilibBuf[bufferIndex + 0] = read & 0xff;
                ilibBuf[bufferIndex + 1] = (read >> 8) & 0xff;
                ilibBuf[bufferIndex + 2] = (read >> 16) & 0xff;
                ilibBuf[bufferIndex + 3] = (read >> 24) & 0xff;
                bufferIndex += 4;
            }

            if (bufferIndex == sizeof(ilibBuf))
            {
                bufferIndex = 0;
                // Toggle LED to show progress reading Flash
                digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                bufferCount += 1;

                int s = NDP.loadLog(ilibBuf, sizeof(ilibBuf));

                // reenable master SPI -- it may have been disabled as the
                // result of a reset performed in the uilib log sequence
                // this is needed to stop master SPI hanging!!!!!
                enableMasterSpi();
            }
        }
        digitalWrite(LED_BUILTIN, LOW);

        for (long i = bufferIndex; i < (int)sizeof(ilibBuf); i++)
        {
            ilibBuf[i] = 0xff;
        }
        // load last buffer
        s = NDP.loadLog(ilibBuf, sizeof(ilibBuf));
        changeMasterSpiMode(MSPI_IDLE);

        // check if last reasponse from uilib is 0x00.
        // This indicates uilib successfully loaded
        if (s == 0 && (record == 0))
        {
            runningFromFlash = 1; // if main record loaded signal runningFromFlash
        }
        else if (record == 1)
        {
            if (s != 0)
                runningFromFlash = 0; // failed to load patch
            else
                patchApplied = 1;
        }
    }

    // Disable Master SPI so we can access NDP LED GPIOs (bug)
    disableMasterSpi();
}
