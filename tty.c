#include "lib.h"

void readline(void);
int uart_readchar(void);

volatile static char *const uart = 0x10000000;

struct plic {
	uint32_t priorities[1024];
	uint32_t pending[32];
	uint32_t empty1[0x3e0];
	uint32_t enable[32 * 15872];
	uint32_t empty2[0x3800];
	struct {
		uint32_t threshold;
		uint32_t claim_complete;
	} contexts[15872];
} __attribute__ ((packed));

volatile static struct plic *const plic = 0xc000000;


void
ttyinit(void)
{
	uart[1] = 1;				/* Enable data interrupt */
	plic->priorities[10]		= 1;		/* Set priority */
	plic->enable[0]			= 1<<10;	/* Enable UART interrupts */
	plic->contexts[0].threshold	= 0;		/* Unmask */
}

char linebuf[128];
int line_i;

int
readchar(void)
{
	int c = peekchar();
	line_i++;
	return c;
}

int
peekchar(void)
{
	if (linebuf[line_i] == 0) readline();
	return linebuf[line_i];
}

void
readline(void)
{
	int c, i = 0;

	while ((c = uart_readchar()) != '\n')
		linebuf[i++] = c;
	linebuf[i] = c;
	linebuf[i + 1] = 0;
	line_i = 0;
}

int __attribute__ ((noinline))
uart_readchar(void)
{
	int lsr, c;
	lsr = uart[5];
	while (!(lsr & 1)) {
		asm volatile ("wfi");
		lsr = uart[5];
	}

	c = uart[0];
	if (c == '\r') {
		uart[0] = c;
		c = '\n';
	}
	uart[0] = c;	/* Echo */
	return c;
}

void __attribute__ ((noinline))
writechar(int c)
{
	if (c == '\n')
		uart[0] = '\r';
	uart[0] = c;
}
