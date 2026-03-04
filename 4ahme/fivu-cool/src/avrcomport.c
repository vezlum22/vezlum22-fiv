#ifdef __AVR__ // Compile this file only when targeting AVR microcontrollers.

/**
 * @file avrcomport.c
 * @brief UART-backed CLI transport implementation for AVR targets.
 */

#include <avr/io.h> // AVR-specific IO register definitions.
#include <avr/interrupt.h> // AVR interrupt handling helpers.
#include <avr/pgmspace.h> // PROGMEM-aware printing helpers.
#include <stdio.h> // Standard IO declarations.
#include <stdarg.h> // va_list handling for vprintf hooks.

#include "cli.h" // CLI transport interface definitions.
#include "avrcomport.h" // AVR comport transport declarations.

/**
 * @brief Holds register pointers and bit masks for an AVR UART instance.
 */
typedef struct { // Encapsulates register pointers and masks for one UART.
    volatile uint8_t *udr; // Pointer to the UART data register.
    volatile uint8_t *ucsra; // Pointer to the UART status register A.
    volatile uint8_t *ucsrb; // Pointer to the UART control register B.
    volatile uint16_t *ubrr; // Pointer to the baud rate register.
    uint8_t udre_mask; // Mask for the data register empty flag.
    uint8_t rxc_mask; // Mask for the receive complete flag.
    uint8_t rxen_mask; // Mask for enabling the receiver.
    uint8_t txen_mask; // Mask for enabling the transmitter.
    uint8_t u2x_mask;  // Mask for the double-speed flag.
} AvrUartCtx; // Alias for the UART context struct.

/**
 * @brief Aggregates UART context with a FILE stream for vfprintf bindings.
 */
typedef struct {
    AvrUartCtx *uart; // Hardware UART register set
    FILE stream;      // FILE stream bound to uart_putchar
    CliComPort *owner; // CLI port owning this transport
} AvrCliCtx;

// Static contexts for available UARTs
#ifdef UBRR0 // Only define UART0 context if the hardware register exists.
static AvrUartCtx uart0_ctx = { &UDR0, &UCSR0A, &UCSR0B, &UBRR0, (1 << UDRE0), (1 << RXC0), (1 << RXEN0), (1 << TXEN0), (1 << U2X0) }; // Context for UART0 registers and masks.
static AvrCliCtx cli0_ctx = { &uart0_ctx, {0}, NULL };
#endif
#ifdef UBRR1 // Only define UART1 context if supported.
static AvrUartCtx uart1_ctx = { &UDR1, &UCSR1A, &UCSR1B, &UBRR1, (1 << UDRE1), (1 << RXC1), (1 << RXEN1), (1 << TXEN1), (1 << U2X1) }; // Context for UART1 registers and masks.
static AvrCliCtx cli1_ctx = { &uart1_ctx, {0}, NULL };
#endif
#ifdef UBRR2 // Only define UART2 context if supported.
static AvrUartCtx uart2_ctx = { &UDR2, &UCSR2A, &UCSR2B, &UBRR2, (1 << UDRE2), (1 << RXC2), (1 << RXEN2), (1 << TXEN2), (1 << U2X2) }; // Context for UART2 registers and masks.
static AvrCliCtx cli2_ctx = { &uart2_ctx, {0}, NULL };
#endif
#ifdef UBRR3 // Only define UART3 context if supported.
static AvrUartCtx uart3_ctx = { &UDR3, &UCSR3A, &UCSR3B, &UBRR3, (1 << UDRE3), (1 << RXC3), (1 << RXEN3), (1 << TXEN3), (1 << U2X3) }; // Context for UART3 registers and masks.
static AvrCliCtx cli3_ctx = { &uart3_ctx, {0}, NULL };
#endif

// Forward declarations for transport callbacks
static int uart_init(void *ctx, uint32_t bps); // Initialize UART with the desired baud rate.
static void uart_send(void *ctx, TxMode mode, uint8_t byte); // Send a single byte over UART.
static uint8_t uart_read(void *ctx, TxMode mode); // Read a single byte from UART.
static uint8_t uart_available(void *ctx); // Check if data is available to read.
static void uart_set_rx_enabled(void *ctx, uint8_t enabled); // Enable or disable UART RX.
static void uart_flush_rx(void *ctx); // Clear pending bytes in RX buffer.
static int uart_vprintf(void *ctx, const char *fmt, va_list ap); // vprintf using FILE stream
static int uart_vprintf_progmem(void *ctx, const char *fmt, va_list ap); // vprintf_P using FILE stream
static int uart_putchar(char c, FILE *stream); // FILE putchar bound to UART
static void setup_stream(AvrCliCtx *cc); // Initialize FILE stream for vfprintf helpers

// Helper to pick a HardwareSerial instance by index
static AvrUartCtx* get_uart_ctx(uint8_t uartId) // Return context matching the UART ID.
{
    switch (uartId) // Select based on UART index.
    {
        #if defined(UBRR0) // Include UART0 case if supported.
        case 0:
            return &uart0_ctx; // Provide UART0 context.
        #endif
        #if defined(UBRR1) // Include UART1 case if supported.
        case 1:
            return &uart1_ctx; // Provide UART1 context.
        #endif
        #if defined(UBRR2) // Include UART2 case if supported.
        case 2:
            return &uart2_ctx; // Provide UART2 context.
        #endif
        #if defined(UBRR3) // Include UART3 case if supported.
        case 3:
            return &uart3_ctx; // Provide UART3 context.
        #endif
        default:
            return NULL; // No matching UART found.
    }
}

// Helper to get FILE-backed CLI context by UART index
static AvrCliCtx* get_cli_ctx(uint8_t uartId)
{
    switch (uartId)
    {
        #if defined(UBRR0)
        case 0: return &cli0_ctx;
        #endif
        #if defined(UBRR1)
        case 1: return &cli1_ctx;
        #endif
        #if defined(UBRR2)
        case 2: return &cli2_ctx;
        #endif
        #if defined(UBRR3)
        case 3: return &cli3_ctx;
        #endif
        default: return NULL;
    }
}

// Transport callback implementations
/**
 * @brief Initialize UART registers with the requested baud rate.
 * @param ctx Pointer to the UART context (AvrUartCtx).
 * @param bps Target baud rate.
 * @return 0 on success, -1 on invalid context.
 */
static int uart_init(void *ctx, uint32_t bps) // Configure UART registers for a given baud rate.
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    AvrUartCtx *c = cc ? cc->uart : NULL; // Cast generic context to UART context.
    if (!c || !c->ubrr || !c->ucsrb) // Validate required register pointers.
        return -1; // Signal failure when context is incomplete.

    // Always use double-speed mode (U2X) to minimize baud error across all rates.
    const uint32_t divisor = 8UL;
    const uint16_t ubrr_value = (uint16_t)(((F_CPU + (bps * divisor / 2)) / (bps * divisor)) - 1); // Rounded divider

    uint8_t sreg = SREG; // Save current interrupt status.
    cli(); // Disable interrupts while touching shared registers.
    if (c->ucsra)
        *(c->ucsra) |= c->u2x_mask;
    *(c->ubrr) = ubrr_value; // Set baud rate register.
    *(c->ucsrb) = c->rxen_mask | c->txen_mask; // Enable RX and TX bits.
    SREG = sreg; // Restore interrupt status.
    return 0; // Indicate successful initialization.
}

/**
 * @brief Send one byte over UART.
 * @param ctx Pointer to the UART context (AvrUartCtx).
 * @param mode Blocking mode selection.
 * @param byte Byte to transmit.
 */
static void uart_send(void *ctx, TxMode mode, uint8_t byte) // Send one byte through UART.
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    AvrUartCtx *c = cc ? cc->uart : NULL; // Cast context to UART context.
    if (!c || !c->ucsra || !c->udr) // Ensure required registers exist.
        return; // Abort if context invalid.
    if (mode == WHEN_READY) // Block until transmitter ready if requested.
        while (!(*(c->ucsra) & c->udre_mask)); // Spin until data register is empty.
    *(c->udr) = byte; // Write byte to data register for transmission.
}

/**
 * @brief Read one byte from UART.
 * @param ctx Pointer to the UART context (AvrUartCtx).
 * @param mode Blocking mode selection.
 * @return Received byte or 0 on invalid context.
 */
static uint8_t uart_read(void *ctx, TxMode mode) // Read one byte from UART.
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    AvrUartCtx *c = cc ? cc->uart : NULL; // Cast context to UART context.
    if (!c || !c->ucsra || !c->udr) // Ensure context is valid.
        return 0; // Return zero when unavailable.
    if (mode == WHEN_READY) // Optionally block until data is ready.
        while (!(*(c->ucsra) & c->rxc_mask)); // Spin until a byte is received.
    return *(c->udr); // Return received byte from data register.
}

/**
 * @brief Check if unread UART data is available.
 * @param ctx Pointer to the UART context (AvrUartCtx).
 * @return 1 if data available, 0 otherwise.
 */
static uint8_t uart_available(void *ctx) // Check if UART has unread data.
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    AvrUartCtx *c = cc ? cc->uart : NULL; // Cast context to UART context.
    if (!c || !c->ucsra) // Validate access to status register.
        return 0; // Indicate no data on invalid context.
    return (*(c->ucsra) & c->rxc_mask) ? 1 : 0; // Return 1 if RX flag is set.
}

/**
 * @brief Enable or disable the UART receiver.
 * @param ctx Pointer to the UART context (AvrUartCtx).
 * @param enabled Non-zero to enable, zero to disable.
 */
static void uart_set_rx_enabled(void *ctx, uint8_t enabled) // Toggle UART receiver state.
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    AvrUartCtx *c = cc ? cc->uart : NULL; // Cast context to UART context.
    if (!c || !c->ucsrb) // Ensure control register is available.
        return; // Abort on invalid context.
    if (enabled) // When enabling receiver...
        *(c->ucsrb) |= c->rxen_mask; // Set RX enable bit.
    else // When disabling receiver...
        *(c->ucsrb) &= ~(c->rxen_mask); // Clear RX enable bit.
}

/**
 * @brief Discard all pending bytes in the UART RX buffer.
 * @param ctx Pointer to the UART context (AvrUartCtx).
 */
static void uart_flush_rx(void *ctx) // Drain all pending RX bytes.
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    AvrUartCtx *c = cc ? cc->uart : NULL; // Cast context to UART context.
    if (!c || !c->ucsra || !c->udr) // Verify registers are valid.
        return; // Exit if context incomplete.
    while (*(c->ucsra) & c->rxc_mask) // Loop while data is pending.
        (void)*(c->udr); // Read and discard each pending byte.
}

// Bind FILE stream to UART putchar once
static void setup_stream(AvrCliCtx *cc)
{
    if (!cc)
        return;
    if (cc->stream.put == NULL)
    {
        fdev_setup_stream(&cc->stream, uart_putchar, NULL, _FDEV_SETUP_WRITE);
        fdev_set_udata(&cc->stream, cc);
    }
}

// Bind CLI port to the transport context
static void uart_bind_port(void *ctx, CliComPort* cliComPort)
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    if (cc)
        cc->owner = cliComPort;
}

// FILE putchar that emits bytes over UART
static int uart_putchar(char c, FILE *stream)
{
    AvrCliCtx *cc = (AvrCliCtx *)fdev_get_udata(stream);
    AvrUartCtx *ctx = cc ? cc->uart : NULL;
    if (!ctx || !ctx->ucsra || !ctx->udr)
        return 0;
    while (!(*(ctx->ucsra) & ctx->udre_mask));
    *(ctx->udr) = (uint8_t)c;
    if (c == '\n' && cc)
        cliIncrementLineFeedCounter(cc->owner);
    return 0;
}

// vprintf using the FILE stream
static int uart_vprintf(void *ctx, const char *fmt, va_list ap)
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    if (!cc)
        return -1;
    setup_stream(cc);
    return vfprintf(&cc->stream, fmt, ap);
}

// vprintf_P using the FILE stream for PROGMEM fmt strings
static int uart_vprintf_progmem(void *ctx, const char *fmt, va_list ap)
{
    AvrCliCtx *cc = (AvrCliCtx *)ctx;
    if (!cc)
        return -1;
    setup_stream(cc);
    return vfprintf_P(&cc->stream, fmt, ap);
}

/**
 * @brief Create a CLI transport that uses the specified AVR UART.
 * @param uartId UART index (0-based).
 * @param bps Desired baud rate.
 * @return Populated CliTransport; callbacks are set when the UART exists.
 */
CliTransport avrcomportCreateUartTx(uint8_t uartId, uint32_t bps) // Build a CLI transport for the requested UART.
{
    AvrUartCtx *ctx = get_uart_ctx(uartId); // Look up context for requested UART ID.
    AvrCliCtx *cliCtx = get_cli_ctx(uartId); // FILE-backed context
    CliTransport t = {0}; // Initialize transport struct to zero.
    if (ctx && cliCtx) // Only proceed if context is valid.
    {
        cliCtx->uart = ctx;
        setup_stream(cliCtx);
        t.ctx = cliCtx; // Store UART/FILE context pointer.
        t.bps = bps; // Store desired baud rate.
        t.init = uart_init; // Assign initialization callback.
        t.send = uart_send; // Assign send callback.
        t.read = uart_read; // Assign read callback.
        t.available = uart_available; // Assign availability check callback.
        t.set_rx_enabled = uart_set_rx_enabled; // Assign RX enable/disable callback.
        t.flush_rx = uart_flush_rx; // Assign RX flush callback.
        t.flush_tx = NULL; // No TX flush implementation.
        t.stream = &cliCtx->stream; // Allow stdout binding on the CLI side
        t.vprintf = uart_vprintf; // Bind std formatting to FILE stream
        t.vprintf_progmem = uart_vprintf_progmem; // Bind PROGMEM formatting
        t.bind_port = uart_bind_port; // Bind CLI port to this transport
    }
    return t; // Return populated or empty transport.
}

#endif // End of AVR guard.
