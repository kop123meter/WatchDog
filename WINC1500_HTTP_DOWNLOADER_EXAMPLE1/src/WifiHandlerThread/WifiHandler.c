/**
 * @file      WifiHandler.c
 * @brief     File to handle HTTP Download and MQTT support
 * @author    Eduardo Garcia
 * @date      2020-01-01

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "WifiHandlerThread/WifiHandler.h"
#include <errno.h>
#include "ControlThread/ControlThread.h"
#include "UiHandlerThread/UiHandlerThread.h"


/******************************************************************************
 * Defines
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/
volatile char mqtt_msg[64] = "{\"d\":{\"temp\":17}}\"";
volatile char mqtt_msg_temp[64] = "{\"d\":{\"temp\":17}}\"";

volatile uint32_t temperature = 1;
int8_t wifiStateMachine = WIFI_MQTT_INIT;   ///< Global variable that determines the state of the WIFI handler.
QueueHandle_t xQueueWifiState = NULL;       ///< Queue to determine the Wifi state from other threads.
QueueHandle_t xQueueGameBuffer = NULL;      ///< Queue to send the next play to the cloud
QueueHandle_t xQueueImuBuffer = NULL;       ///< Queue to send IMU data to the cloud
QueueHandle_t xQueueDistanceBuffer = NULL;  ///< Queue to send the distance to the cloud

/*HTTP DOWNLOAD RELATED DEFINES AND VARIABLES*/

uint8_t do_download_flag = false;  // Flag that when true initializes a download. False to connect to MQTT broker
/** File download processing state. */
static download_state down_state = NOT_READY;
/** SD/MMC mount. */
static FATFS fatfs;
/** File pointer for file download. */
static FIL file_object;
/** Http content length. */
static uint32_t http_file_size = 0;
/** Receiving content length. */
static uint32_t received_file_size = 0;
/** File name to download. */
static char save_file_name[MAIN_MAX_FILE_NAME_LENGTH + 1] = "0:";

/** UART module for debug. */
// static struct usart_module cdc_uart_module;

/** Instance of Timer module. */
struct sw_timer_module swt_module_inst;

/** Instance of HTTP client module. */
struct http_client_module http_client_module_inst;

/*MQTT RELATED DEFINES AND VARIABLES*/

/** User name of chat. */
char mqtt_user[64] = "Unit1";

/* Instance of MQTT service. */
static struct mqtt_module mqtt_inst;

/* Receive buffer of the MQTT service. */
static unsigned char mqtt_read_buffer[MAIN_MQTT_BUFFER_SIZE];
static unsigned char mqtt_send_buffer[MAIN_MQTT_BUFFER_SIZE];

/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static void MQTT_InitRoutine(void);
static void MQTT_HandleGameMessages(void);
static void MQTT_HandleImuMessages(void);
static void HTTP_DownloadFileInit(void);
static void HTTP_DownloadFileTransaction(void);
/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/*HTPP RELATED STATIOC FUNCTIONS*/

/**
 * \brief Initialize download state to not ready.
 */
static void init_state(void)
{
    down_state = NOT_READY;
}

/**
 * \brief Clear state parameter at download processing state.
 * \param[in] mask Check download_state.
 */
static void clear_state(download_state mask)
{
    down_state &= ~mask;
}

/**
 * \brief Add state parameter at download processing state.
 * \param[in] mask Check download_state.
 */
static void add_state(download_state mask)
{
    down_state |= mask;
}

/**
 * \brief File download processing state check.
 * \param[in] mask Check download_state.
 * \return true if this state is set, false otherwise.
 */

static inline bool is_state_set(download_state mask)
{
    return ((down_state & mask) != 0);
}

/**
 * \brief File existing check.
 * \param[in] fp The file pointer to check.
 * \param[in] file_path_name The file name to check.
 * \return true if this file name is exist, false otherwise.
 */
static bool is_exist_file(FIL *fp, const char *file_path_name)
{
    if (fp == NULL || file_path_name == NULL) {
        return false;
    }

    FRESULT ret = f_open(&file_object, (char const *)file_path_name, FA_OPEN_EXISTING);
    f_close(&file_object);
    return (ret == FR_OK);
}

/**
 * \brief Make to unique file name.
 * \param[in] fp The file pointer to check.
 * \param[out] file_path_name The file name change to uniquely and changed name is returned to this buffer.
 * \param[in] max_len Maximum file name length.
 * \return true if this file name is unique, false otherwise.
 */
static bool rename_to_unique(FIL *fp, char *file_path_name, uint8_t max_len)
{
#define NUMBRING_MAX (3)
#define ADDITION_SIZE (NUMBRING_MAX + 1) /* '-' character is added before the number. */
    uint16_t i = 1, name_len = 0, ext_len = 0, count = 0;
    char name[MAIN_MAX_FILE_NAME_LENGTH + 1] = {0};
    char ext[MAIN_MAX_FILE_EXT_LENGTH + 1] = {0};
    char numbering[NUMBRING_MAX + 1] = {0};
    char *p = NULL;
    bool valid_ext = false;

    if (file_path_name == NULL) {
        return false;
    }

    if (!is_exist_file(fp, file_path_name)) {
        return true;
    } else if (strlen(file_path_name) > MAIN_MAX_FILE_NAME_LENGTH) {
        return false;
    }

    p = strrchr(file_path_name, '.');
    if (p != NULL) {
        ext_len = strlen(p);
        if (ext_len < MAIN_MAX_FILE_EXT_LENGTH) {
            valid_ext = true;
            strcpy(ext, p);
            if (strlen(file_path_name) - ext_len > MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE) {
                name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE - ext_len;
                strncpy(name, file_path_name, name_len);
            } else {
                name_len = (p - file_path_name);
                strncpy(name, file_path_name, name_len);
            }
        } else {
            name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE;
            strncpy(name, file_path_name, name_len);
        }
    } else {
        name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE;
        strncpy(name, file_path_name, name_len);
    }

    name[name_len++] = '-';

    for (i = 0, count = 1; i < NUMBRING_MAX; i++) {
        count *= 10;
    }
    for (i = 1; i < count; i++) {
        sprintf(numbering, MAIN_ZERO_FMT(NUMBRING_MAX), i);
        strncpy(&name[name_len], numbering, NUMBRING_MAX);
        if (valid_ext) {
            strcpy(&name[name_len + NUMBRING_MAX], ext);
        }

        if (!is_exist_file(fp, name)) {
            memset(file_path_name, 0, max_len);
            strcpy(file_path_name, name);
            return true;
        }
    }
    return false;
}

/**
 * \brief Start file download via HTTP connection.
 */
static void start_download(void)
{
    if (!is_state_set(STORAGE_READY)) {
        LogMessage(LOG_DEBUG_LVL, "start_download: MMC storage not ready.\r\n");
        return;
    }

    if (!is_state_set(WIFI_CONNECTED)) {
        LogMessage(LOG_DEBUG_LVL, "start_download: Wi-Fi is not connected.\r\n");
        return;
    }

    if (is_state_set(GET_REQUESTED)) {
        LogMessage(LOG_DEBUG_LVL, "start_download: request is sent already.\r\n");
        return;
    }

    if (is_state_set(DOWNLOADING)) {
        LogMessage(LOG_DEBUG_LVL, "start_download: running download already.\r\n");
        return;
    }

    /* Send the HTTP request. */
    LogMessage(LOG_DEBUG_LVL, "start_download: sending HTTP request...\r\n");
    http_client_send_request(&http_client_module_inst, MAIN_HTTP_FILE_URL, HTTP_METHOD_GET, NULL, NULL);
}

/**
 * \brief Store received packet to file.
 * \param[in] data Packet data.
 * \param[in] length Packet data length.
 */
static void store_file_packet(char *data, uint32_t length)
{
    FRESULT ret;
    if ((data == NULL) || (length < 1)) {
        LogMessage(LOG_DEBUG_LVL, "store_file_packet: empty data.\r\n");
        return;
    }

    if (!is_state_set(DOWNLOADING)) {
        char *cp = NULL;
        save_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
        save_file_name[1] = ':';
        cp = (char *)(MAIN_HTTP_FILE_URL + strlen(MAIN_HTTP_FILE_URL));
        while (*cp != '/') {
            cp--;
        }
        if (strlen(cp) > 1) {
            cp++;
            strcpy(&save_file_name[2], cp);
        } else {
            LogMessage(LOG_DEBUG_LVL, "store_file_packet: file name is invalid. Download canceled.\r\n");
            add_state(CANCELED);
            return;
        }

        rename_to_unique(&file_object, save_file_name, MAIN_MAX_FILE_NAME_LENGTH);
        LogMessage(LOG_DEBUG_LVL, "store_file_packet: creating file [%s]\r\n", save_file_name);
        ret = f_open(&file_object, (char const *)save_file_name, FA_CREATE_ALWAYS | FA_WRITE);
        if (ret != FR_OK) {
            LogMessage(LOG_DEBUG_LVL, "store_file_packet: file creation error! ret:%d\r\n", ret);
            return;
        }

        received_file_size = 0;
        add_state(DOWNLOADING);
    }

    if (data != NULL) {
        UINT wsize = 0;
        ret = f_write(&file_object, (const void *)data, length, &wsize);
        if (ret != FR_OK) {
            f_close(&file_object);
            add_state(CANCELED);
            LogMessage(LOG_DEBUG_LVL, "store_file_packet: file write error, download canceled.\r\n");
            return;
        }

        received_file_size += wsize;
        LogMessage(LOG_DEBUG_LVL, "store_file_packet: received[%lu], file size[%lu]\r\n", (unsigned long)received_file_size, (unsigned long)http_file_size);
        if (received_file_size >= http_file_size) {
            f_close(&file_object);
            LogMessage(LOG_DEBUG_LVL, "store_file_packet: file downloaded successfully.\r\n");
            port_pin_set_output_level(LED_0_PIN, false);
            add_state(COMPLETED);
            return;
        }
    }
}

/**
 * \brief Callback of the HTTP client.
 *
 * \param[in]  module_inst     Module instance of HTTP client module.
 * \param[in]  type            Type of event.
 * \param[in]  data            Data structure of the event. \refer http_client_data
 */
static void http_client_callback(struct http_client_module *module_inst, int type, union http_client_data *data)
{
    switch (type) {
        case HTTP_CLIENT_CALLBACK_SOCK_CONNECTED:
            LogMessage(LOG_DEBUG_LVL, "http_client_callback: HTTP client socket connected.\r\n");
            break;

        case HTTP_CLIENT_CALLBACK_REQUESTED:
            LogMessage(LOG_DEBUG_LVL, "http_client_callback: request completed.\r\n");
            add_state(GET_REQUESTED);
            break;

        case HTTP_CLIENT_CALLBACK_RECV_RESPONSE:
            LogMessage(LOG_DEBUG_LVL, "http_client_callback: received response %u data size %u\r\n", (unsigned int)data->recv_response.response_code, (unsigned int)data->recv_response.content_length);
            if ((unsigned int)data->recv_response.response_code == 200) {
                http_file_size = data->recv_response.content_length;
                received_file_size = 0;
            } else {
                add_state(CANCELED);
                return;
            }
            if (data->recv_response.content_length <= MAIN_BUFFER_MAX_SIZE) {
                store_file_packet(data->recv_response.content, data->recv_response.content_length);
                add_state(COMPLETED);
            }
            break;

        case HTTP_CLIENT_CALLBACK_RECV_CHUNKED_DATA:
            store_file_packet(data->recv_chunked_data.data, data->recv_chunked_data.length);
            if (data->recv_chunked_data.is_complete) {
                add_state(COMPLETED);
            }

            break;

        case HTTP_CLIENT_CALLBACK_DISCONNECTED:
            LogMessage(LOG_DEBUG_LVL, "http_client_callback: disconnection reason:%d\r\n", data->disconnected.reason);

            /* If disconnect reason is equal to -ECONNRESET(-104),
             * It means the server has closed the connection (timeout).
             * This is normal operation.
             */
            if (data->disconnected.reason == -EAGAIN) {
                /* Server has not responded. Retry immediately. */
                if (is_state_set(DOWNLOADING)) {
                    f_close(&file_object);
                    clear_state(DOWNLOADING);
                }

                if (is_state_set(GET_REQUESTED)) {
                    clear_state(GET_REQUESTED);
                }

                start_download();
            }

            break;
    }
}

/**
 * \brief Callback to get the data from socket.
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg socket event type. Possible values are:
 *  - SOCKET_MSG_BIND
 *  - SOCKET_MSG_LISTEN
 *  - SOCKET_MSG_ACCEPT
 *  - SOCKET_MSG_CONNECT
 *  - SOCKET_MSG_RECV
 *  - SOCKET_MSG_SEND
 *  - SOCKET_MSG_SENDTO
 *  - SOCKET_MSG_RECVFROM
 * \param[in] pvMsg is a pointer to message structure. Existing types are:
 *  - tstrSocketBindMsg
 *  - tstrSocketListenMsg
 *  - tstrSocketAcceptMsg
 *  - tstrSocketConnectMsg
 *  - tstrSocketRecvMsg
 */
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
    http_client_socket_event_handler(sock, u8Msg, pvMsg);
}

/**
 * \brief Callback for the gethostbyname function (DNS Resolution callback).
 * \param[in] pu8DomainName Domain name of the host.
 * \param[in] u32ServerIP Server IPv4 address encoded in NW byte order format. If it is Zero, then the DNS resolution failed.
 */
static void resolve_cb(uint8_t *pu8DomainName, uint32_t u32ServerIP)
{
    LogMessage(LOG_DEBUG_LVL,
               "resolve_cb: %s IP address is %d.%d.%d.%d\r\n\r\n",
               pu8DomainName,
               (int)IPV4_BYTE(u32ServerIP, 0),
               (int)IPV4_BYTE(u32ServerIP, 1),
               (int)IPV4_BYTE(u32ServerIP, 2),
               (int)IPV4_BYTE(u32ServerIP, 3));
    http_client_socket_resolve_handler(pu8DomainName, u32ServerIP);
}

/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CURRENT_RSSI](@ref M2M_WIFI_RESP_CURRENT_RSSI)
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_CONNTION_STATE](@ref M2M_WIFI_RESP_CONNTION_STATE)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_WPS](@ref M2M_WIFI_REQ_WPS)
 *  - [M2M_WIFI_RESP_IP_CONFIGURED](@ref M2M_WIFI_RESP_IP_CONFIGURED)
 *  - [M2M_WIFI_RESP_IP_CONFLICT](@ref M2M_WIFI_RESP_IP_CONFLICT)
 *  - [M2M_WIFI_RESP_P2P](@ref M2M_WIFI_RESP_P2P)
 *  - [M2M_WIFI_RESP_AP](@ref M2M_WIFI_RESP_AP)
 *  - [M2M_WIFI_RESP_CLIENT_INFO](@ref M2M_WIFI_RESP_CLIENT_INFO)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type. Existing types are:
 *  - tstrM2mWifiStateChanged
 *  - tstrM2MWPSInfo
 *  - tstrM2MP2pResp
 *  - tstrM2MAPResp
 *  - tstrM2mScanDone
 *  - tstrM2mWifiscanResult
 */
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType) {
        case M2M_WIFI_RESP_CON_STATE_CHANGED: {
            tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
            if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
                LogMessage(LOG_DEBUG_LVL, "wifi_cb: M2M_WIFI_CONNECTED\r\n");
                m2m_wifi_request_dhcp_client();
            } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
                LogMessage(LOG_DEBUG_LVL, "wifi_cb: M2M_WIFI_DISCONNECTED\r\n");
                clear_state(WIFI_CONNECTED);
                if (is_state_set(DOWNLOADING)) {
                    f_close(&file_object);
                    clear_state(DOWNLOADING);
                }

                if (is_state_set(GET_REQUESTED)) {
                    clear_state(GET_REQUESTED);
                }

                /* Disconnect from MQTT broker. */
                /* Force close the MQTT connection, because cannot send a disconnect message to the broker when network is broken. */
                mqtt_disconnect(&mqtt_inst, 1);

                m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
            }

            break;
        }

        case M2M_WIFI_REQ_DHCP_CONF: {
            uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
            LogMessage(LOG_DEBUG_LVL, "wifi_cb: IP address is %u.%u.%u.%u\r\n", pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
            add_state(WIFI_CONNECTED);

            if (do_download_flag == 1) {
                start_download();

            } else {
                /* Try to connect to MQTT broker when Wi-Fi was connected. */
                if (mqtt_connect(&mqtt_inst, main_mqtt_broker)) {
                    LogMessage(LOG_DEBUG_LVL, "Error connecting to MQTT Broker!\r\n");
                }
            }
        } break;

        default:
            break;
    }
}

/**
 * \brief Initialize SD/MMC storage.
 */
void init_storage(void)
{
    FRESULT res;
    Ctrl_status status;

    /* Initialize SD/MMC stack. */
    sd_mmc_init();
    while (true) {
        LogMessage(LOG_DEBUG_LVL, "init_storage: please plug an SD/MMC card in slot...\r\n");

        /* Wait card present and ready. */
        do {
            status = sd_mmc_test_unit_ready(0);
            if (CTRL_FAIL == status) {
                LogMessage(LOG_DEBUG_LVL, "init_storage: SD Card install failed.\r\n");
                LogMessage(LOG_DEBUG_LVL, "init_storage: try unplug and re-plug the card.\r\n");
                while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
                }
            }
        } while (CTRL_GOOD != status);

        LogMessage(LOG_DEBUG_LVL, "init_storage: mounting SD card...\r\n");
        memset(&fatfs, 0, sizeof(FATFS));
        res = f_mount(LUN_ID_SD_MMC_0_MEM, &fatfs);
        if (FR_INVALID_DRIVE == res) {
            LogMessage(LOG_DEBUG_LVL, "init_storage: SD card mount failed! (res %d)\r\n", res);
            return;
        }

        LogMessage(LOG_DEBUG_LVL, "init_storage: SD card mount OK.\r\n");
        add_state(STORAGE_READY);
        return;
    }
}

/**
 * \brief Configure UART console.
 */
/*static void configure_console(void)
{

        stdio_base = (void *)GetUsartModule();
        ptr_put = (int (*)(void volatile*,char))&usart_serial_putchar;
        ptr_get = (void (*)(void volatile*,char*))&usart_serial_getchar;


        # if defined(__GNUC__)
        // Specify that stdout and stdin should not be buffered.
        setbuf(stdout, NULL);
        setbuf(stdin, NULL);
        // Note: Already the case in IAR's Normal DLIB default configuration
        // and AVR GCC library:
        // - printf() emits one character at a time.
        // - getchar() requests only 1 byte to exit.
        #  endif
        //stdio_serial_init(GetUsartModule(), EDBG_CDC_MODULE, &usart_conf);
        //usart_enable(&cdc_uart_module);
}*/

/**
 * \brief Configure Timer module.
 */
static void configure_timer(void)
{
    struct sw_timer_config swt_conf;
    sw_timer_get_config_defaults(&swt_conf);

    sw_timer_init(&swt_module_inst, &swt_conf);
    sw_timer_enable(&swt_module_inst);
}

/**
 * \brief Configure HTTP client module.
 */
static void configure_http_client(void)
{
    struct http_client_config httpc_conf;
    int ret;

    http_client_get_config_defaults(&httpc_conf);

    httpc_conf.recv_buffer_size = MAIN_BUFFER_MAX_SIZE;
    httpc_conf.timer_inst = &swt_module_inst;
    httpc_conf.port = 443;
    httpc_conf.tls = 1;

    ret = http_client_init(&http_client_module_inst, &httpc_conf);
    if (ret < 0) {
        LogMessage(LOG_DEBUG_LVL, "configure_http_client: HTTP client initialization failed! (res %d)\r\n", ret);
        while (1) {
        } /* Loop forever. */
    }

    http_client_register_callback(&http_client_module_inst, http_client_callback);
}

/*MQTT RELATED STATIC FUNCTIONS*/

/** Prototype for MQTT subscribe Callback */
void SubscribeHandler(MessageData *msgData);

/**
 * \brief Callback to get the Socket event.
 *
 * \param[in] Socket descriptor.
 * \param[in] msg_type type of Socket notification. Possible types are:
 *  - [SOCKET_MSG_CONNECT](@ref SOCKET_MSG_CONNECT)
 *  - [SOCKET_MSG_BIND](@ref SOCKET_MSG_BIND)
 *  - [SOCKET_MSG_LISTEN](@ref SOCKET_MSG_LISTEN)
 *  - [SOCKET_MSG_ACCEPT](@ref SOCKET_MSG_ACCEPT)
 *  - [SOCKET_MSG_RECV](@ref SOCKET_MSG_RECV)
 *  - [SOCKET_MSG_SEND](@ref SOCKET_MSG_SEND)
 *  - [SOCKET_MSG_SENDTO](@ref SOCKET_MSG_SENDTO)
 *  - [SOCKET_MSG_RECVFROM](@ref SOCKET_MSG_RECVFROM)
 * \param[in] msg_data A structure contains notification informations.
 */
static void socket_event_handler(SOCKET sock, uint8_t msg_type, void *msg_data)
{
    mqtt_socket_event_handler(sock, msg_type, msg_data);
}

/**
 * \brief Callback of gethostbyname function.
 *
 * \param[in] doamin_name Domain name.
 * \param[in] server_ip IP of server.
 */
static void socket_resolve_handler(uint8_t *doamin_name, uint32_t server_ip)
{
    mqtt_socket_resolve_handler(doamin_name, server_ip);
}

/**
 * \brief Callback to receive the subscribed Message.
 *
 * \param[in] msgData Data to be received.
 */

void SubscribeHandlerLedTopic(MessageData *msgData)
{
    uint8_t rgb[3] = {0, 0, 0};
    LogMessage(LOG_DEBUG_LVL, "\r\n %.*s", msgData->topicName->lenstring.len, msgData->topicName->lenstring.data);
    // Will receive something of the style "rgb(222, 224, 189)"
    if (strncmp(msgData->message->payload, "rgb(", 4) == 0) {
        char *p = (char *)&msgData->message->payload[4];
        int nb = 0;
        while (nb <= 2 && *p) {
            rgb[nb++] = strtol(p, &p, 10);
            if (*p != ',') break;
            p++; /* skip, */
        }
        LogMessage(LOG_DEBUG_LVL, "\r\nRGB %d %d %d\r\n", rgb[0], rgb[1], rgb[2]);
        UIChangeColors(rgb[0], rgb[1], rgb[2]);
    }
}

void SubscribeHandlerGameTopic(MessageData *msgData)
{
    struct GameDataPacket game;
    memset(game.game, 0xff, sizeof(game.game));

    // Parse input. The start string must be '{"game":['
    if (strncmp(msgData->message->payload, "{\"game\":[", 9) == 0) {
        LogMessage(LOG_DEBUG_LVL, "\r\nGame message received!\r\n");
        LogMessage(LOG_DEBUG_LVL, "\r\n %.*s", msgData->topicName->lenstring.len, msgData->topicName->lenstring.data);
        LogMessage(LOG_DEBUG_LVL, "%.*s", msgData->message->payloadlen, (char *)msgData->message->payload);

        int nb = 0;
        char *p = &msgData->message->payload[9];
        while (nb < GAME_SIZE && *p) {
            game.game[nb++] = strtol(p, &p, 10);
            if (*p != ',') break;
            p++; /* skip, */
        }
        LogMessage(LOG_DEBUG_LVL, "\r\nParsed Command: ");
        for (int i = 0; i < GAME_SIZE; i++) {
            LogMessage(LOG_DEBUG_LVL, "%d,", game.game[i]);
        }

        if (pdTRUE == ControlAddGameData(&game)) {
            LogMessage(LOG_DEBUG_LVL, "\r\nSent play to control!\r\n");
        }

    } else {
        LogMessage(LOG_DEBUG_LVL, "\r\nGame message received but not understood!\r\n");
        LogMessage(LOG_DEBUG_LVL, "\r\n %.*s", msgData->topicName->lenstring.len, msgData->topicName->lenstring.data);
        LogMessage(LOG_DEBUG_LVL, "%.*s", msgData->message->payloadlen, (char *)msgData->message->payload);
    }
}

void SubscribeHandlerImuTopic(MessageData *msgData)
{
	LogMessage(LOG_DEBUG_LVL, "\r\nIMU topic received!\r\n");
    LogMessage(LOG_DEBUG_LVL, "\r\n %.*s", msgData->topicName->lenstring.len, msgData->topicName->lenstring.data);
}

void SubscribeHandlerDistanceTopic(MessageData *msgData)
{
	LogMessage(LOG_DEBUG_LVL, "\r\nDistance topic received!\r\n");
    LogMessage(LOG_DEBUG_LVL, "\r\n %.*s", msgData->topicName->lenstring.len, msgData->topicName->lenstring.data);
}

void SubscribeHandler(MessageData *msgData)
{
    /* You received publish message which you had subscribed. */
    /* Print Topic and message */
    LogMessage(LOG_DEBUG_LVL, "\r\n %.*s", msgData->topicName->lenstring.len, msgData->topicName->lenstring.data);
    LogMessage(LOG_DEBUG_LVL, " >> ");
    LogMessage(LOG_DEBUG_LVL, "%.*s", msgData->message->payloadlen, (char *)msgData->message->payload);

    // Handle LedData message
    if (strncmp((char *)msgData->topicName->lenstring.data, LED_TOPIC, msgData->message->payloadlen) == 0) {
        if (strncmp((char *)msgData->message->payload, LED_TOPIC_LED_OFF, msgData->message->payloadlen) == 0) {
            port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
        } else if (strncmp((char *)msgData->message->payload, LED_TOPIC_LED_ON, msgData->message->payloadlen) == 0) {
            port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
        }
    }
}

/**
 * \brief Callback to get the MQTT status update.
 *
 * \param[in] conn_id instance id of connection which is being used.
 * \param[in] type type of MQTT notification. Possible types are:
 *  - [MQTT_CALLBACK_SOCK_CONNECTED](@ref MQTT_CALLBACK_SOCK_CONNECTED)
 *  - [MQTT_CALLBACK_CONNECTED](@ref MQTT_CALLBACK_CONNECTED)
 *  - [MQTT_CALLBACK_PUBLISHED](@ref MQTT_CALLBACK_PUBLISHED)
 *  - [MQTT_CALLBACK_SUBSCRIBED](@ref MQTT_CALLBACK_SUBSCRIBED)
 *  - [MQTT_CALLBACK_UNSUBSCRIBED](@ref MQTT_CALLBACK_UNSUBSCRIBED)
 *  - [MQTT_CALLBACK_DISCONNECTED](@ref MQTT_CALLBACK_DISCONNECTED)
 *  - [MQTT_CALLBACK_RECV_PUBLISH](@ref MQTT_CALLBACK_RECV_PUBLISH)
 * \param[in] data A structure contains notification informations. @ref mqtt_data
 */
static void mqtt_callback(struct mqtt_module *module_inst, int type, union mqtt_data *data)
{
    switch (type) {
        case MQTT_CALLBACK_SOCK_CONNECTED: {
            /*
             * If connecting to broker server is complete successfully, Start sending CONNECT message of MQTT.
             * Or else retry to connect to broker server.
             */
            if (data->sock_connected.result >= 0) {
                LogMessage(LOG_DEBUG_LVL, "\r\nConnecting to Broker...");
                if (0 != mqtt_connect_broker(module_inst, 1, CLOUDMQTT_USER_ID, CLOUDMQTT_USER_PASSWORD, CLOUDMQTT_USER_ID, NULL, NULL, 0, 0, 0)) {
                    LogMessage(LOG_DEBUG_LVL, "MQTT  Error - NOT Connected to broker\r\n");
                } else {
                    LogMessage(LOG_DEBUG_LVL, "MQTT Connected to broker\r\n");
                }
            } else {
                LogMessage(LOG_DEBUG_LVL, "Connect fail to server(%s)! retry it automatically.\r\n", main_mqtt_broker);
                mqtt_connect(module_inst, main_mqtt_broker); /* Retry that. */
            }
        } break;

        case MQTT_CALLBACK_CONNECTED:
            if (data->connected.result == MQTT_CONN_RESULT_ACCEPT) {
                /* Subscribe chat topic. */
                mqtt_subscribe(module_inst, GAME_TOPIC_IN, 2, SubscribeHandlerGameTopic);
                mqtt_subscribe(module_inst, LED_TOPIC, 2, SubscribeHandlerLedTopic);
                mqtt_subscribe(module_inst, IMU_TOPIC, 2, SubscribeHandlerImuTopic);
                /* Enable USART receiving callback. */

                LogMessage(LOG_DEBUG_LVL, "MQTT Connected\r\n");
            } else {
                /* Cannot connect for some reason. */
                LogMessage(LOG_DEBUG_LVL, "MQTT broker decline your access! error code %d\r\n", data->connected.result);
            }

            break;

        case MQTT_CALLBACK_DISCONNECTED:
            /* Stop timer and USART callback. */
            LogMessage(LOG_DEBUG_LVL, "MQTT disconnected\r\n");
            // usart_disable_callback(&cdc_uart_module, USART_CALLBACK_BUFFER_RECEIVED);
            break;
    }
}

/**
 * \brief Configure MQTT service.
 */
static void configure_mqtt(void)
{
    struct mqtt_config mqtt_conf;
    int result;

    mqtt_get_config_defaults(&mqtt_conf);
    /* To use the MQTT service, it is necessary to always set the buffer and the timer. */
    mqtt_conf.read_buffer = mqtt_read_buffer;
    mqtt_conf.read_buffer_size = MAIN_MQTT_BUFFER_SIZE;
    mqtt_conf.send_buffer = mqtt_send_buffer;
    mqtt_conf.send_buffer_size = MAIN_MQTT_BUFFER_SIZE;
    mqtt_conf.port = CLOUDMQTT_PORT;
    mqtt_conf.keep_alive = 6000;

    result = mqtt_init(&mqtt_inst, &mqtt_conf);
    if (result < 0) {
        LogMessage(LOG_DEBUG_LVL, "MQTT initialization failed. Error code is (%d)\r\n", result);
        while (1) {
        }
    }

    result = mqtt_register_callback(&mqtt_inst, mqtt_callback);
    if (result < 0) {
        LogMessage(LOG_DEBUG_LVL, "MQTT register callback failed. Error code is (%d)\r\n", result);
        while (1) {
        }
    }
}

// SETUP FOR EXTERNAL BUTTON INTERRUPT -- Used to send an MQTT Message

void configure_extint_channel(void)
{
    struct extint_chan_conf config_extint_chan;
    extint_chan_get_config_defaults(&config_extint_chan);
    config_extint_chan.gpio_pin = BUTTON_0_EIC_PIN;
    config_extint_chan.gpio_pin_mux = BUTTON_0_EIC_MUX;
    config_extint_chan.gpio_pin_pull = EXTINT_PULL_UP;
    config_extint_chan.detection_criteria = EXTINT_DETECT_FALLING;
    extint_chan_set_config(BUTTON_0_EIC_LINE, &config_extint_chan);
}

void extint_detection_callback(void);
void configure_extint_callbacks(void)
{
    extint_register_callback(extint_detection_callback, BUTTON_0_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
    extint_chan_enable_callback(BUTTON_0_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
}

volatile bool isPressed = false;
void extint_detection_callback(void)
{
    // Publish some data after a button press and release. Note: just an example! This is not the most elegant way of doing this!
    temperature++;
    if (temperature > 40) temperature = 1;
    snprintf(mqtt_msg_temp, 63, "{\"d\":{\"temp\":%d}}", temperature);
    isPressed = true;
    //Published in the Wifi thread main loop
}

/**
 static void HTTP_DownloadFileInit(void)
 * @brief	Routine to initialize HTTP download of the OTAU file
 * @note

*/
static void HTTP_DownloadFileInit(void)
{
    if (mqtt_disconnect(&mqtt_inst, main_mqtt_broker)) {
        LogMessage(LOG_DEBUG_LVL, "Error connecting to MQTT Broker!\r\n");
    }
    while ((mqtt_inst.isConnected)) {
        m2m_wifi_handle_events(NULL);
    }
    socketDeinit();
    // DOWNLOAD A FILE
    do_download_flag = true;
    /* Register socket callback function. */
    registerSocketCallback(socket_cb, resolve_cb);
    /* Initialize socket module. */
    socketInit();

    start_download();
    wifiStateMachine = WIFI_DOWNLOAD_HANDLE;
}

/**
 static void HTTP_DownloadFileTransaction(void)
 * @brief	Routine to handle the HTTP transaction of downloading a file
 * @note

*/
static void HTTP_DownloadFileTransaction(void)
{
    /* Connect to router. */
    while (!(is_state_set(COMPLETED) || is_state_set(CANCELED))) {
        /* Handle pending events from network controller. */
        m2m_wifi_handle_events(NULL);
        /* Checks the timer timeout. */
        sw_timer_task(&swt_module_inst);
        vTaskDelay(5);
    }

    // Disable socket for HTTP Transfer
    socketDeinit();
    vTaskDelay(1000);
    // CONNECT TO MQTT BROKER
    do_download_flag = false;

    // Write Flag
    char test_file_name[] = "0:FlagA.txt";
    test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
    FRESULT res = f_open(&file_object, (char const *)test_file_name, FA_CREATE_ALWAYS | FA_WRITE);

    if (res != FR_OK) {
        LogMessage(LOG_INFO_LVL, "[FAIL] res %d\r\n", res);
    } else {
        SerialConsoleWriteString("FlagA.txt added!\r\n");
    }
	
	f_close(&file_object);
    wifiStateMachine = WIFI_MQTT_INIT;
}

/**
 static void MQTT_InitRoutine(void)
 * @brief	Routine to initialize the MQTT socket to prepare for MQTT transactions
 * @note

*/
static void MQTT_InitRoutine(void)
{
    socketDeinit();
    configure_mqtt();
    // Re-enable socket for MQTT Transfer
    registerSocketCallback(socket_event_handler, socket_resolve_handler);
    socketInit();
    /* Connect to router. */
    if (!(mqtt_inst.isConnected)) {
        if (mqtt_connect(&mqtt_inst, main_mqtt_broker)) {
            LogMessage(LOG_DEBUG_LVL, "Error connecting to MQTT Broker!\r\n");
        }
    }

    if (mqtt_inst.isConnected) {
        LogMessage(LOG_DEBUG_LVL, "Connected to MQTT Broker!\r\n");
    }
    wifiStateMachine = WIFI_MQTT_HANDLE;
}

/**
 static void MQTT_HandleTransactions(void)
 * @brief	Routine to handle MQTT transactions
 * @note

*/
static void MQTT_HandleTransactions(void)
{
    /* Handle pending events from network controller. */
    m2m_wifi_handle_events(NULL);
    sw_timer_task(&swt_module_inst);

    // Check if data has to be sent!
    MQTT_HandleGameMessages();
    MQTT_HandleImuMessages();

    // Handle MQTT messages
    if (mqtt_inst.isConnected) mqtt_yield(&mqtt_inst, 100);
}

static void MQTT_HandleImuMessages(void)
{
    struct ImuDataPacket imuDataVar;
    if (pdPASS == xQueueReceive(xQueueImuBuffer, &imuDataVar, 0)) {
        snprintf(mqtt_msg, 63, "{\"imux\":%d, \"imuy\": %d, \"imuz\": %d}", imuDataVar.xmg, imuDataVar.ymg, imuDataVar.zmg);
        mqtt_publish(&mqtt_inst, IMU_TOPIC, mqtt_msg, strlen(mqtt_msg), 1, 0);
    }
}

static void MQTT_HandleGameMessages(void)
{
    struct GameDataPacket gamePacket;
    if (pdPASS == xQueueReceive(xQueueGameBuffer, &gamePacket, 0)) {
        snprintf(mqtt_msg, 63, "{\"game\":[");
        for (int iter = 0; iter < GAME_SIZE; iter++) {
            char numGame[5];
            if (gamePacket.game[iter] != 0xFF) {
                snprintf(numGame, 3, "%d", gamePacket.game[iter]);
                strcat(mqtt_msg, numGame);
                if (gamePacket.game[iter + 1] != 0xFF && iter + 1 < GAME_SIZE) {
                    snprintf(numGame, 5, ",");
                    strcat(mqtt_msg, numGame);
                }
            } else {
                break;
            }
        }
        strcat(mqtt_msg, "]}");
        LogMessage(LOG_DEBUG_LVL, mqtt_msg);
        LogMessage(LOG_DEBUG_LVL, "\r\n");
        mqtt_publish(&mqtt_inst, GAME_TOPIC_OUT, mqtt_msg, strlen(mqtt_msg), 1, 0);
    }
}
/**
 * \brief Main application function.
 *
 * Application entry point.
 *
 * \return program return value.
 */
void vWifiTask(void *pvParameters)
{
    tstrWifiInitParam param;
    int8_t ret;
    vTaskDelay(100);
    init_state();
    // Create buffers to send data
    xQueueWifiState = xQueueCreate(5, sizeof(uint32_t));
    xQueueImuBuffer = xQueueCreate(5, sizeof(struct ImuDataPacket));
    xQueueGameBuffer = xQueueCreate(2, sizeof(struct GameDataPacket));
    xQueueDistanceBuffer = xQueueCreate(5, sizeof(uint16_t));

    if (xQueueWifiState == NULL || xQueueImuBuffer == NULL || xQueueGameBuffer == NULL || xQueueDistanceBuffer == NULL) {
        SerialConsoleWriteString("ERROR Initializing Wifi Data queues!\r\n");
    }

    SerialConsoleWriteString("ESE516 - Wifi Init Code\r\n");
    /* Initialize the Timer. */
    configure_timer();

    /* Initialize the HTTP client service. */
    configure_http_client();

    /* Initialize the MQTT service. */
    configure_mqtt();

    /* Initialize SD/MMC storage. */
    //init_storage();

    /*Initialize BUTTON 0 as an external interrupt*/
    configure_extint_channel();
    configure_extint_callbacks();

    /* Initialize Wi-Fi parameters structure. */
    memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

    nm_bsp_init();

    /* Initialize Wi-Fi driver with data and status callbacks. */
    param.pfAppWifiCb = wifi_cb;
    ret = m2m_wifi_init(&param);
    if (M2M_SUCCESS != ret) {
        LogMessage(LOG_DEBUG_LVL, "main: m2m_wifi_init call error! (res %d)\r\n", ret);
        while (1) {
        }
    }

    LogMessage(LOG_DEBUG_LVL, "main: connecting to WiFi AP %s...\r\n", (char *)MAIN_WLAN_SSID);

    // Re-enable socket for MQTT Transfer
    socketInit();
    registerSocketCallback(socket_event_handler, socket_resolve_handler);

    m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);

    while (!(is_state_set(WIFI_CONNECTED))) {
        /* Handle pending events from network controller. */
        m2m_wifi_handle_events(NULL);
        /* Checks the timer timeout. */
        sw_timer_task(&swt_module_inst);
    }

    vTaskDelay(1000);

    wifiStateMachine = WIFI_MQTT_HANDLE;
    while (1) {
        switch (wifiStateMachine) {
            case (WIFI_MQTT_INIT): {
                MQTT_InitRoutine();

                break;
            }

            case (WIFI_MQTT_HANDLE): {
                MQTT_HandleTransactions();
                break;
            }

            case (WIFI_DOWNLOAD_INIT): {
                HTTP_DownloadFileInit();
                break;
            }

            case (WIFI_DOWNLOAD_HANDLE): {
                HTTP_DownloadFileTransaction();
                break;
            }

            default:
                wifiStateMachine = WIFI_MQTT_INIT;
                break;
        }
        // Check if a new state was called
        uint8_t DataToReceive = 0;
        if (pdPASS == xQueueReceive(xQueueWifiState, &DataToReceive, 0)) {
            wifiStateMachine = DataToReceive;  // Update new state
        }

        //Check if we need to publish something. In this example, we publish the "temperature" when the button was pressed.
        if(isPressed)
        {
            mqtt_publish(&mqtt_inst, TEMPERATURE_TOPIC, mqtt_msg_temp, strlen(mqtt_msg_temp), 1, 0);
            LogMessage(LOG_DEBUG_LVL, "MQTT send %s\r\n", mqtt_msg_temp);
            isPressed = false;

        }

        vTaskDelay(100);
    }
    return;
}

void WifiHandlerSetState(uint8_t state)
{
    if (state <= WIFI_DOWNLOAD_HANDLE) {
        xQueueSend(xQueueWifiState, &state, (TickType_t)10);
    }
}

/**
 void WifiAddImuDataToQueue(struct ImuDataPacket* imuPacket)
 * @brief	Adds an IMU struct to the queue to send via MQTT
 * @param[out]

 * @return		Returns pdTrue if data can be added to queue, pdFalse if queue is full
 * @note

*/
int WifiAddImuDataToQueue(struct ImuDataPacket *imuPacket)
{
    int error = xQueueSend(xQueueImuBuffer, imuPacket, (TickType_t)10);
    return error;
}

/**
 void WifiAddImuDataToQueue(struct ImuDataPacket* imuPacket)
 * @brief	Adds an Distance data to the queue to send via MQTT
 * @param[out]

 * @return		Returns pdTrue if data can be added to queue, pdFalse if queue is full
 * @note

*/
int WifiAddDistanceDataToQueue(uint16_t *distance)
{
    int error = xQueueSend(xQueueDistanceBuffer, distance, (TickType_t)10);
    return error;
}

/**
 void WifiAddGameToQueue(struct ImuDataPacket* imuPacket)
 * @brief	Adds an game to the queue to send via MQTT. Game data must have 0xFF IN BYTES THAT WILL NOT BE SENT!
 * @param[out]

 * @return		Returns pdTrue if data can be added to queue, pdFalse if queue is full
 * @note

*/
int WifiAddGameDataToQueue(struct GameDataPacket *game)
{
    int error = xQueueSend(xQueueGameBuffer, game, (TickType_t)10);
    return error;
}
