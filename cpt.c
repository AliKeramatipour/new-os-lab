#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char buf[512];
int main(int argc, char *argv[])
{
	
	//one file
	if ( argc > 3 || argc < 2 )
	{
		printf(1,"number of arguments not valid\n") ;
		exit() ;
	}
	if ( argc == 2 )
	{
		int id1 = open(argv[1], O_CREATE | O_WRONLY) ;
		int len = read(0, buf, sizeof(buf)) ;
		write(id1, buf, len) ;
		exit() ;
	}
	
	int id1 = open(argv[1], O_RDONLY) ;
	if (id1 == -1 )
	{
		printf(1,"file not found\n");
		exit() ;
	}
	
	int id2 = open(argv[2], O_CREATE | O_WRONLY) ;
	int len ;
	while( (len = read(id1, buf, sizeof(buf))) > 0 )
	{
		write(id2,buf,len) ;
	}
	exit();
}
