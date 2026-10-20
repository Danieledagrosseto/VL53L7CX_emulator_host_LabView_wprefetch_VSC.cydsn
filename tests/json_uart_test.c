#include <assert.h>
#include <setjmp.h>

#define main firmware_main
#include "..\main.c"
#undef main

static const char scanRequest[] =
    "{\"type\":\"configuration\",\"command\":\"scan_i2c\",\"id\":1}";
static char output[4096];
static uint8_t slaves[128];
static uint8_t failedAddress = 0u;
static uint8_t stopStatus = I2C_MSTR_NO_ERROR;
static uint8_t lastAddress;
static bool stopNotReadyAfterNak;
static unsigned int probeCount;
static uint8_t inputData;
static uint8_t inputStatus;
static bool inputPending;
static jmp_buf startupFinished;
static unsigned int allocationCount;
static unsigned int failAllocation;
static unsigned int outstandingAllocations;

uint8 mock_uart_status(void)
{
    return inputPending ? (uint8)(inputStatus | UART_RX_STS_FIFO_NOTEMPTY) : 0u;
}

uint8 mock_uart_data(void)
{
    inputPending = false;
    return inputData;
}

uint8 I2C_MasterSendStart(uint8 address, uint8 mode)
{
    assert(mode == I2C_WRITE_XFER_MODE);
    assert(address >= I2C_FIRST_ADDRESS && address <= I2C_LAST_ADDRESS);
    probeCount++;
    lastAddress = address;
    if(address == failedAddress)
    {
        return I2C_MSTR_BUS_BUSY;
    }
    return slaves[address] ? I2C_MSTR_NO_ERROR : I2C_MSTR_ERR_LB_NAK;
}

uint8 I2C_MasterSendStop(void)
{
    if(stopNotReadyAfterNak && !slaves[lastAddress])
    {
        return I2C_MSTR_NOT_READY;
    }
    return stopStatus;
}

void UART_PutString(const char *text)
{
    assert(strlen(output) + strlen(text) < sizeof(output));
    strcat(output, text);
}

void SW_Running_LED_Write(uint8 value)
{
    assert(value == 1u);
    longjmp(startupFinished, 1);
}

uint8_t vl53l7cx_init(VL53L7CX_Configuration *dev)
{
    (void)dev;
    return 0u;
}

uint8_t vl53l7cx_is_alive(VL53L7CX_Configuration *dev, uint8_t *alive)
{
    (void)dev;
    *alive = 1u;
    return 0u;
}

static void feed_byte(uint8_t data, uint8_t status)
{
    inputData = data;
    inputStatus = status;
    inputPending = true;
    RxIsr();
    assert(!inputPending);
}

static void feed(const char *text)
{
    while(*text != '\0')
    {
        feed_byte((uint8_t)*text++, 0u);
    }
}

static cJSON *consume(void)
{
    char command[MAX_CMD_SIZE];
    uint16_t status;
    cJSON *response;
    size_t length;

    assert(Isnewcommand);
    output[0] = '\0';
    status = uart_take_command(command, sizeof(command));
    assert(!Isnewcommand);
    json_process_command(command, status);
    length = strlen(output);
    assert(length >= 2u);
    assert(strcmp(output + length - 2u, "\r\n") == 0);
    assert(strchr(output, '\n') == output + length - 1u);
    response = cJSON_ParseWithOpts(output, NULL, true);
    assert(cJSON_IsObject(response));
    return response;
}

static cJSON *send_line(const char *text)
{
    feed(text);
    feed("\r\n");
    return consume();
}

static void expect_error(cJSON *response, const char *error)
{
    assert(strcmp(cJSON_GetObjectItemCaseSensitive(response, "status")->valuestring,
                  "error") == 0);
    assert(strcmp(cJSON_GetObjectItemCaseSensitive(response, "error")->valuestring,
                  error) == 0);
    assert(!cJSON_HasObjectItem(response, "addresses"));
    cJSON_Delete(response);
}

static void *test_malloc(size_t size)
{
    void *pointer;
    if(allocationCount++ == failAllocation)
    {
        return NULL;
    }
    pointer = malloc(size);
    if(pointer != NULL)
    {
        outstandingAllocations++;
    }
    return pointer;
}

static void test_free(void *pointer)
{
    if(pointer != NULL)
    {
        assert(outstandingAllocations > 0u);
        outstandingAllocations--;
    }
    free(pointer);
}

int main(void)
{
    cJSON *response;
    cJSON *addresses;
    cJSON *request;
    char padded[258];
    const char *invalidIds[] = {
        "{}", "{\"id\":null}", "{\"id\":\"1\"}", "{\"id\":-1}",
        "{\"id\":1.5}", "{\"id\":9007199254740992}", "{\"id\":1e999}"
    };

    slaves[0x29] = 1u;
    slaves[0x70] = 1u;
    if(setjmp(startupFinished) == 0)
    {
        (void)firmware_main();
        assert(false);
    }
    assert(output[0] == '\0');
    assert(probeCount == I2C_ADDRESS_COUNT);
    assert(savedAddressCount == 2u);
    assert(savedAddresses8bit[0] == 82u && savedAddresses8bit[1] == 224u);

    response = send_line(scanRequest);
    assert(strcmp(output, "{\"type\":\"configuration\",\"command\":\"scan_i2c\","
        "\"id\":1,\"status\":\"ok\",\"addresses\":[82,224]}\r\n") == 0);
    assert(probeCount == 2u * I2C_ADDRESS_COUNT);
    cJSON_Delete(response);

    memset(slaves, 0, sizeof(slaves));
    stopNotReadyAfterNak = true;
    response = send_line(scanRequest);
    assert(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(response, "addresses")) == 0);
    assert(savedAddressCount == 0u);
    cJSON_Delete(response);
    stopNotReadyAfterNak = false;

    memset(slaves, 1, sizeof(slaves));
    response = send_line(scanRequest);
    addresses = cJSON_GetObjectItemCaseSensitive(response, "addresses");
    assert(cJSON_GetArraySize(addresses) == I2C_ADDRESS_COUNT);
    for(unsigned int i = 0u; i < I2C_ADDRESS_COUNT; i++)
    {
        assert(cJSON_GetArrayItem(addresses, (int)i)->valueint ==
               (int)((i + I2C_FIRST_ADDRESS) << 1));
    }
    cJSON_Delete(response);

    failedAddress = 0x30u;
    expect_error(send_line(scanRequest), "i2c_scan_failed");
    assert(savedAddressCount == 0u);
    failedAddress = 0u;
    stopStatus = I2C_MSTR_BUS_BUSY;
    expect_error(send_line(scanRequest), "i2c_scan_failed");
    stopStatus = I2C_MSTR_NO_ERROR;

    expect_error(send_line("not json"), "invalid_json");
    expect_error(send_line(""), "invalid_json");
    expect_error(send_line("[]"), "invalid_request");
    expect_error(send_line("1"), "invalid_request");
    expect_error(send_line("{\"id\":1}"), "invalid_request");
    expect_error(send_line("{\"type\":\"configuration\",\"command\":\"other\",\"id\":1}"),
                 "unsupported_command");
    expect_error(send_line("{\"type\":\"Configuration\",\"command\":\"scan_i2c\",\"id\":1}"),
                 "unsupported_command");
    for(size_t i = 0u; i < sizeof(invalidIds) / sizeof(invalidIds[0]); i++)
    {
        response = send_line(invalidIds[i]);
        assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(response, "id")));
        expect_error(response, "invalid_id");
    }
    expect_error(send_line("{\"id\":1} trailing"), "invalid_json");
    response = send_line("{\"type\":\"configuration\",\"command\":\"scan_i2c\","
                         "\"id\":9007199254740991}");
    assert(cJSON_GetObjectItemCaseSensitive(response, "id")->valuedouble == 9007199254740991.0);
    cJSON_Delete(response);

    memset(padded, ' ', sizeof(padded));
    memcpy(padded, scanRequest, strlen(scanRequest));
    padded[MAX_JSON_SIZE] = '\0';
    response = send_line(padded);
    assert(strcmp(cJSON_GetObjectItemCaseSensitive(response, "status")->valuestring, "ok") == 0);
    cJSON_Delete(response);
    feed(padded);
    feed("\n");
    cJSON_Delete(consume());
    padded[MAX_JSON_SIZE] = ' ';
    padded[MAX_JSON_SIZE + 1u] = '\0';
    expect_error(send_line(padded), "command_too_long");
    for(unsigned int i = 0u; i < 1000u; i++)
    {
        feed("x");
    }
    feed("\r\n");
    expect_error(consume(), "command_too_long");
    response = send_line(scanRequest);
    assert(strcmp(cJSON_GetObjectItemCaseSensitive(response, "status")->valuestring, "ok") == 0);
    cJSON_Delete(response);

    feed(scanRequest);
    assert(!Isnewcommand);
    feed_byte(0u, 0u);
    feed("\r\n");
    expect_error(consume(), "invalid_character");
    feed_byte('{', UART_RX_STS_OVERRUN);
    feed("\r\n");
    expect_error(consume(), "uart_receive_error");
    cJSON_Delete(send_line(scanRequest));

    feed(scanRequest);
    feed("\r\n");
    feed("must not overwrite pending command\r\n");
    response = consume();
    assert(strcmp(cJSON_GetObjectItemCaseSensitive(response, "status")->valuestring, "ok") == 0);
    cJSON_Delete(response);

    request = cJSON_Parse(scanRequest);
    assert(request != NULL);
    for(failAllocation = 0u; failAllocation < 24u; failAllocation++)
    {
        cJSON_Hooks hooks = {test_malloc, test_free};
        allocationCount = 0u;
        output[0] = '\0';
        cJSON_InitHooks(&hooks);
        json_send_response(request, NULL);
        cJSON_InitHooks(NULL);
        assert(outstandingAllocations == 0u);
        response = cJSON_ParseWithOpts(output, NULL, true);
        assert(response != NULL);
        if(failAllocation < allocationCount)
        {
            expect_error(response, "out_of_memory");
        }
        else
        {
            assert(strcmp(cJSON_GetObjectItemCaseSensitive(response, "status")->valuestring, "ok") == 0);
            cJSON_Delete(response);
        }
    }
    cJSON_Delete(request);
    puts("JSON/UART tests passed (startup, scans, validation, framing, recovery, allocation failures).");
    return 0;
}
