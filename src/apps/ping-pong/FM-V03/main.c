#include <string.h>
#include "board.h"
#include "gpio.h"
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "utilities.h"
#include "printf.h"

#define RF_FREQUENCY                                475500000 // Hz 475.5MHz

#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       8         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 64 // Define the payload size here

static uint16_t BufferSize = BUFFER_SIZE;
static uint8_t Buffer[BUFFER_SIZE];

static int8_t RssiValue = 0;
static int8_t SnrValue = 0;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

#define APP_TX_DUTYCYCLE                            5000
#define APP_TX_DUTYCYCLE_RND                        1000

static uint32_t TxDutyCycleTime;

static TimerEvent_t TxNextPacketTimer;

static bool NextTx = true;

static enum eDeviceState
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_SLEEP
} DeviceState;

/*!
 * LED GPIO pins objects
 */
extern Gpio_t Led1;
extern Gpio_t Led2;

static void OnTxDone( void )
{
    printf("## F: %s\r\n", __func__);
    Radio.Sleep( );
    GpioWrite( &Led1, 1);
}

static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    printf("## F: %s\r\n", __func__);
    Radio.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    printf("Rx %d bytes: %.*s\r\n", BufferSize, BufferSize, Buffer);
}

static void OnTxTimeout( void )
{
    printf("## F: %s\r\n", __func__);
    Radio.Sleep( );
    GpioWrite( &Led1, 1);
}

static void OnRxTimeout( void )
{
    printf("## F: %s\r\n", __func__);
    Radio.Sleep( );
}

static void OnRxError( void )
{
    printf("## F: %s\r\n", __func__);
    Radio.Sleep( );
}

static void OnTxNextPacketTimerEvent( void )
{
    printf("## %s\r\n", __func__);

    TimerStop( &TxNextPacketTimer );

    DeviceState = DEVICE_STATE_SEND;
    NextTx = true;
}

static bool SendFrame(void)
{
    static uint32_t count = 0;

    GpioWrite( &Led1, 0);

    ++count;

    BufferSize = snprintf((char *)Buffer, sizeof(Buffer), "%d\r\n", count);

    Radio.Send( Buffer, BufferSize );

    return true;
}

/**
 * Main application entry point.
 */
int main( void )
{
    // Target board initialization
    BoardInitMcu( );
    BoardInitPeriph( );

    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    Radio.Init( &RadioEvents );

    printf("Semetch ID Version: 0x%02x\r\n", Radio.Read(0x42));
    printf("Semetch Syncword: 0x%02x\r\n", Radio.Read(0x39));

    Radio.SetChannel( RF_FREQUENCY );

    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                       LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                       LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                       true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                       LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                       0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

    TimerInit( &TxNextPacketTimer, OnTxNextPacketTimerEvent );
    DeviceState = DEVICE_STATE_INIT;

    while( 1 )
    {
        switch( DeviceState )
        {

        case DEVICE_STATE_INIT:
            {
                DeviceState = DEVICE_STATE_SEND;
                break;
            }
        case DEVICE_STATE_SEND:
            {
                if( NextTx == true )
                {
                    NextTx = SendFrame( );
                }

                TxDutyCycleTime = APP_TX_DUTYCYCLE + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
                DeviceState = DEVICE_STATE_CYCLE;
                break;
            }
        case DEVICE_STATE_CYCLE:
            {
                DeviceState = DEVICE_STATE_SLEEP;

                // Schedule next packet transmission
                TimerSetValue( &TxNextPacketTimer, TxDutyCycleTime );
                TimerStart( &TxNextPacketTimer );
                break;
            }
        case DEVICE_STATE_SLEEP:
            {
                // Wake up through events
                TimerLowPowerHandler( );
                break;
            }
        default:
            {
                DeviceState = DEVICE_STATE_INIT;
                break;
            }
        }
    }

    return 0;
}
