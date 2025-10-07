#include <stdbool.h>

extern void setup(void);
extern void loop(void);

int main(void)
{
	setup();

	while (true) {
		loop();
	}

	return 0;
}
