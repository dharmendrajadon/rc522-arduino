#include <Arduino.h>

/*
 * Write personal data of a MIFARE RFID card using a RFID-RC522 reader
 * Uses MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT. 
 ----------------------------------------------------------------------------- 
 * Pin layout should be as follows:
 * Signal     Pin              Pin               Pin
 *            Arduino Uno      Arduino Mega      MFRC522 board
 * ------------------------------------------------------------
 * Reset      9                5                 RST
 * SPI SS     10               53                SDA
 * SPI MOSI   11               52                MOSI
 * SPI MISO   12               51                MISO
 * SPI SCK    13               50                SCK
 *
 * Hardware required:
 * Arduino
 * PCD (Proximity Coupling Device): NXP MFRC522 Contactless Reader IC
 * PICC (Proximity Integrated Circuit Card): A card or tag using the ISO 14443A interface, eg Mifare or NTAG203.
 * The reader can be found on eBay for around 5 dollars. Search for "mf-rc522" on ebay.com. 
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Keyboard.h>

#define SS_PIN 10 //We are using Arduino Pro Micro
#define RST_PIN 9

/*
 * Initialize all variables and configurations
 */
short int buzzer = 9;       // Pin of buzzer module
short int switchType = 8;   // Pin of buzzer module
short int block;            // block number
MFRC522::StatusCode status; // Status response from MFRC522
MFRC522::MIFARE_Key key;    // Mifare card key for read/write

MFRC522 mfrcObject(SS_PIN, RST_PIN); // Create MFRC522 instance
short int type = 1;

void setup()
{
    pinMode(buzzer, OUTPUT);    // Set pinmode for buzzer
    pinMode(switchType, INPUT); // Set pinmode for button switch
    Serial.begin(9600);         // Initialize serial communications with the PC
    SPI.begin();                // Initialize SPI bus
    mfrcObject.PCD_Init();      // Initialize MFRC522 card
    // Enhance the MFRC522 Receiver Gain to maximum value of some 48 dB
    mfrcObject.PCD_SetRegisterBitMask(mfrcObject.RFCfgReg, (0x07<<4));

    // Set ket, by default all keys are set to FFFFFFFFFFFFh at chip delivery from the factory
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    // We are using Block 1 to read and write our data
    block = 1;
}

void loop()
{
    noTone(buzzer); // Put buzzer to silent

    // Look for new cards
    if (!mfrcObject.PICC_IsNewCardPresent())
    {
        return; // If no card found, return from here and do not execute code below this statement
    }

    // Select one of the cards
    if (!mfrcObject.PICC_ReadCardSerial())
    {
        return; // If unable to read card serial, return from here and do not execute code below this statement
    }

    type = digitalRead(switchType);
    switch (type)
    {
    case 1:
    {
        // Initialize buffers
        byte writeBuffer[16];
        byte maxWritableDigits = 16;

        Serial.setTimeout(30000L); // wait until 30 seconds for input from serial
        // Ask Aadhar Number
        tone(buzzer, 2690L); // Start buzzer Tone

        Serial.println("Enter Aadhar Number to write");

        // Authentication for writting on card
        status = mfrcObject.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrcObject.uid));
        if (status != MFRC522::STATUS_OK)
        {
            Serial.println("Authentication Failed");
            Serial.println(mfrcObject.GetStatusCodeName(status));
            return; // If unable to authenticate card using default key, return from here and do not execute code below this statement
        }

        // Read data from serial input
        byte dataLength = Serial.readBytesUntil('#', (char *)writeBuffer, maxWritableDigits);
        for (byte i = dataLength; i < maxWritableDigits; i++)
        {
            writeBuffer[i] = ' '; // pad with spaces in number less than 16
        }

        // Write block
        status = mfrcObject.MIFARE_Write(block, writeBuffer, maxWritableDigits); // Write the buffer to block
        if (status != MFRC522::STATUS_OK)
        {
            Serial.println("Unable to write data to the card");
            Serial.println(mfrcObject.GetStatusCodeName(status));
            break; // If unable to write data to card, return from here and do not execute code below this statement
        }
        else
        {
            Serial.println("Aadhar card number written to card successfully");
        }
        noTone(buzzer);

        break;
    }
    default:
    {
        // Initialize buffers
        byte readBuffer[18];
        byte maxReadableDigits = 18;

        // Authentication for writting on card
        status = mfrcObject.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrcObject.uid));
        if (status != MFRC522::STATUS_OK)
        {
            Serial.println("Authentication Failed");
            Serial.println(mfrcObject.GetStatusCodeName(status));
            return; // If unable to authenticate card using default key, return from here and do not execute code below this statement
        }

        //Read block
        tone(buzzer, 2690L, 3000L);
        status = mfrcObject.MIFARE_Read(block, readBuffer, &maxReadableDigits);
        if (status != MFRC522::STATUS_OK)
        {
            Serial.println(F("Unable to read data to the card."));
            Serial.println(mfrcObject.GetStatusCodeName(status));
            break; // If unable to read data from card, return from here and do not execute code below this statement
        }
        // Begin as Keyboard for connected serial device(to send Card data to input field at cursor position)
        Keyboard.begin();
        //Print Block
        for (int i = 0; i < 16; i++)
        {
            if (readBuffer[i] != 32)
            {
                Keyboard.write(readBuffer[i]);
            }
        }
        Keyboard.end();
        break;
    }
    }

    mfrcObject.PICC_HaltA();      // Halt PICC
    mfrcObject.PCD_StopCrypto1(); // Stop encryption on PCD
}