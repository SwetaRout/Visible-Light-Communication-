#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <unistd.h>

#define GPIO_BASE  0x3F200000
#define TIMER_BASE 0x3F003000
#define INT_BASE 0x3F00B000

#define BITDELAY 500 //in microseconds
#define PACKETSIZE 50

#define INP_GPIO(g)   *(gpio + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio + ((g)/10)) |=  (1<<(((g)%10)*3))

volatile unsigned *gpio,*gpset,*gpclr,*gpin,*timer,*intrupt;

#define CLKHI *gpset = (1 << 18)  // GPIO 18
#define CLKLO *gpclr = (1 << 18)

int setup(void);
int interrupts(int flag);
void startLifi(char *buffer, int packetSize);

void StartLifi(char *buffer, int packetSize)
{
   int n;
   int i;
   unsigned int timend;

   CLKHI;

   interrupts(0);    //Disable interrupts

   timend = *timer + 1000; // set up 10us delay from
   while( (((*timer)-timend) & 0x80000000) != 0);

   timend = *timer + BITDELAY;

   CLKLO;

   timend = *timer + BITDELAY * 2;   // set up 10us delay from
   while( (((*timer)-timend) & 0x80000000) != 0);


   timend = *timer + BITDELAY;

   CLKHI;

   for(n = 0 ; n < packetSize; ++n)
   {
     for(i = 0; i < 8; i++)
     {
       //  delay to timend
       while( (((*timer)-timend) & 0x80000000) != 0);

       if(buffer[n] & 0x01)
       {
         CLKHI;
       }
       else
       {
         CLKLO;
       }

       buffer[n] = buffer[n] >> 1;
       timend += BITDELAY;
     }
   }

   CLKLO;

   interrupts(1);         // re-enable interrupts
}

main()
{
   int socklp1, socklp2, portno, n;

   struct sockaddr_in lap1_addr;
   struct hostent *laptop1;
   const char laptop1HostName[] = "APL-C02SD0YHG8WM.local"; //Laptop #1 IP

   char buffer[256];
   portno = 12345;
   socklp1 = socket(AF_INET, SOCK_STREAM, 0);
   if (socklp1 < 0)
       error("ERROR opening socket");
   laptop1 = gethostbyname(laptop1HostName);
   if (laptop1 == NULL) {
       fprintf(stderr,"ERROR, no such host\n");
       exit(0);
   }
   bzero((char *) &lap1_addr, sizeof(lap1_addr));
   lap1_addr.sin_family = AF_INET;
   bcopy((char *)laptop1->h_addr,
        (char *)&lap1_addr.sin_addr.s_addr,
        laptop1->h_length);
   lap1_addr.sin_port = htons(portno);
   if (connect(socklp1,(struct sockaddr *)&lap1_addr,sizeof(lap1_addr)) < 0)
       error("ERROR connecting");

   struct sockaddr_in lap2_addr;
   struct hostent *laptop2;
   const char laptop2HostName[] = "APL-C02SD0YHG8WM.local"; //Laptop #2 IP

   portno = 12344;
   socklp2 = socket(AF_INET, SOCK_STREAM, 0);
   if (socklp2 < 0)
       error("ERROR opening socket");
   laptop2 = gethostbyname(laptop2HostName);
   if (laptop2 == NULL) {
       fprintf(stderr,"ERROR, no such host\n");
       exit(0);
   }
   bzero((char *) &lap2_addr, sizeof(lap2_addr));
   lap2_addr.sin_family = AF_INET;
   bcopy((char *)laptop2->h_addr,
        (char *)&lap2_addr.sin_addr.s_addr,
        laptop2->h_length);
   lap2_addr.sin_port = htons(portno);
   if (connect(socklp2,(struct sockaddr *)&lap2_addr,sizeof(lap2_addr)) < 0)
       error("ERROR connecting");

   setup();     // setup GPIO, timer and interrupt pointers

   while(1)
   {
      bzero(buffer, 256);
      n = read(socklp1, buffer, 255);
      if(n < 0)
         error("ERROR reading from socket");
      printf(buffer);

      sleep(0.1);    // need to have a small delay here to allow
                     // interrupts to finish

      StartLifi(&buffer, PACKETSIZE);

      sleep(0.1);

      bzero(buffer, 256);
      n = read(socklp2, buffer, 255); //Receive ACK from laptop #2
      if(n < 0)
         error("ERROR reading from socket");

      n = write(socklp1,buffer,1);
    }
   return;
}

int interrupts(int flag)
  {
  static unsigned int sav132 = 0;
  static unsigned int sav133 = 0;
  static unsigned int sav134 = 0;

  if(flag == 0)    // disable
    {
    if(sav132 != 0)
      {
      // Interrupts already disabled so avoid printf
      return(0);
      }

    if( (*(intrupt+128) | *(intrupt+129) | *(intrupt+130)) != 0)
      {
      printf("Pending interrupts\n");  // may be OK but probably
      return(0);                       // better to wait for the
      }                                // pending interrupts to
                                       // clear

    sav134 = *(intrupt+134);
    *(intrupt+137) = sav134;
    sav132 = *(intrupt+132);  // save current interrupts
    *(intrupt+135) = sav132;  // disable active interrupts
    sav133 = *(intrupt+133);
    *(intrupt+136) = sav133;
    }
  else            // flag = 1 enable
    {
    if(sav132 == 0)
      {
      printf("Interrupts not disabled\n");
      return(0);
      }

    *(intrupt+132) = sav132;    // restore saved interrupts
    *(intrupt+133) = sav133;
    *(intrupt+134) = sav134;
    sav132 = 0;                 // indicates interrupts enabled
    }
  return(1);
  }

int setup()
  {
  int memfd;
  unsigned int timend;
  void *gpio_map,*timer_map,*int_map;

  memfd = open("/dev/mem",O_RDWR|O_SYNC);
  if(memfd < 0)
    {
    printf("Mem open error\n");
    return(0);
    }

  gpio_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,
                  MAP_SHARED,memfd,GPIO_BASE);

  timer_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,
                  MAP_SHARED,memfd,TIMER_BASE);

  int_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,
                  MAP_SHARED,memfd,INT_BASE);

  close(memfd);

  if(gpio_map == MAP_FAILED ||
     timer_map == MAP_FAILED ||
     int_map == MAP_FAILED)
    {
    printf("Map failed\n");
    return(0);
    }
              // interrupt pointer
  intrupt = (volatile unsigned *)int_map;
              // timer pointer
  timer = (volatile unsigned *)timer_map;
  ++timer;    // timer lo 4 bytes
              // timer hi 4 bytes available via *(timer+1)

              // GPIO pointers
  gpio = (volatile unsigned *)gpio_map;
  gpset = gpio + 7;     // set bit register offset 28
  gpclr = gpio + 10;    // clr bit register
  gpin = gpio + 13;     // read all bits register

  INP_GPIO(18);
  OUT_GPIO(18);

  return(1);
  }
