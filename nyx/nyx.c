#include <nyx/movegen.h>
#include <nyx/attacks.h>
#include <nyx/uci.h>
#include <nyx/types.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	(void) argc, (void) argv;

	attacks_init();
	movegen_init();

	uci_loop();

	return EXIT_SUCCESS;
}
