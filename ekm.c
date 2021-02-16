// 2014-11-20 Joe Hughes ekm@skydoo.net
// This program gets RS-485 data from an EKM OmniMeter v4
// Written for a Raspberry Pi with an FTDI USB dongle
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>

#define SERIAL_BUF_SIZE (256)
#define MSG_BUF_SIZE (128)

#define DEBUG 0

struct _ekmv4reply {
        char                     start;
        char                     model[2];
        char                     firmware;
        char                     address[12];
        char                     totalkwh[8];
        char                     totalvar[8];
        char                     totalrev[8];
        char                     tpkwh[24];
        char                     tprevkwh[24];
        char                     resettable_kwh[8];
        char                     resettable_rev_kwh[8];
        char                     v1[4];
        char                     v2[4];
        char                     v3[4];
        char                     a1[5];
        char                     a2[5];
        char                     a3[5];
        char                     w1[7];
        char                     w2[7];
        char                     w3[7];
        char                     wtot[7];
        char                     pf[3][4];
        char                     var[4][7];
        char                     freq[4];
        char                     pc[3][8];
        char                     pulse_h_l[1];
        char                     cur_dir[1];
        char                     out_on_off[1];
        char                     kwh_dec[1];
        char                     reserved1[2];
        char                     curtime[14];
        char                     pad[2];
        char                     end[4];
        u_int16_t                crc;
} __attribute__ ((packed));

static const uint16_t const ekmCrcLut[256] = {
0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

uint16_t ekmCheckCrc( const uint8_t const*dat, uint16_t len ) {
  uint16_t crc = 0xffff;

  while (len--) {
    crc = (crc >> 8) ^ ekmCrcLut[(crc ^ *dat++) & 0xff];
  }

  // apparently don't need to swap these on Raspberry
  //crc = (crc << 8) | (crc >> 8);
  crc &= 0x7f7f;

  return crc;
}

int initSerial(char *ttyPath){

  int tty_fd;
  struct termios tio;
   
  memset(&tio,0,sizeof(tio));
  tio.c_iflag=0;
  tio.c_oflag=0;
  tio.c_cflag=CS7|CREAD|CLOCAL|PARENB;           // 8n1, see termios.h for more information
  tio.c_lflag=0;
  tio.c_cc[VMIN]=1;
  tio.c_cc[VTIME]=5;

  tty_fd = open(ttyPath, O_RDWR );      
  cfsetospeed(&tio,B9600);
  cfsetispeed(&tio,B9600);

  tcsetattr(tty_fd,TCSANOW,&tio);

  return(tty_fd);
}


int main(){
  char *ttyPath = "/dev/ttyUSB0";
  int tty_fd;
  int i=0,j=0;
   
  uint8_t c[SERIAL_BUF_SIZE];
  uint8_t d[253];

  struct _ekmv4reply e;

  int red;
  uint16_t crc;
  uint16_t crc1;
  uint16_t crc2;
  char ctotkwh[9];
  char cpulse1[9];
  char cpulse2[9];
  char ctotwatts[8];
  char check[3];

  // add null terminations for atoi
  memset(ctotkwh,'\0',9);
  memset(cpulse1,'\0',9);
  memset(cpulse2,'\0',9);
  memset(ctotwatts,'\0',9);
  memset(check,'\0',9);

  int totkwh = 0;
  int pulse1 = 0;
  int pulse2 = 0;
  int totwatts = 0;

  //Save terminal configuration
  struct termios oldtio;
  tcgetattr(STDOUT_FILENO, &oldtio);
   
  tty_fd = initSerial(ttyPath);
      
  if(tty_fd == -1) {
    printf("error opening serial %s.\n", ttyPath);
    return -1;
  }

  write (tty_fd, "\x2f\x3f\x39\x39\x39\x39\x39\x39\x39\x39\x39\x39\x39\x39\x30\x30\x21\x0d\x0a\n", 19);
  //fsync(tty_fd);
   
  usleep ((7 + 25) * 10000);
  red = read(tty_fd, &c, SERIAL_BUF_SIZE);
  if (DEBUG) {
    for ( i = 0; i < sizeof c; i++) {
      printf(" %02x", c[i]);
    }
    putchar('\n');
    putchar('\n');
  }

  memcpy(check,&c[253],2);

  memcpy(d,&c[1],252);
  memcpy(&e,&c[0],255);

  crc = ekmCheckCrc(d, 252); 
  crc1 = crc & 0x00ff;
  crc2 = crc & 0xff00;
  crc2 = crc2 / 256;
  if (DEBUG) printf("crc:  %02x %02x\n", c[253],c[254]);
  if (DEBUG) printf("calc: %02x %02x\n", crc1,crc2);

  if (c[253] == crc1 && c[254] == crc2) {
    if (DEBUG) printf ("CRC check OK\n");
  } else {
    if (DEBUG) printf ("ERROR CRC check failed\n");
    return -1;
  }

  if (red!=255) {
    if (DEBUG) printf ("ERROR: short read (<255)\n");
    return -2;
  }

  memcpy(ctotkwh,&c[16],8);
  totkwh = atoi(ctotkwh);

  memcpy(cpulse1,&c[203],8);
  pulse1 = atoi(cpulse1);

  memcpy(cpulse2,&c[211],8);
  pulse2 = atoi(cpulse2);

  memcpy(ctotwatts,&e.wtot,7);
  totwatts = atoi(ctotwatts);

  printf("%d %d %d %d\n",totkwh,pulse1,pulse2, totwatts);

  //Close connections
  close(tty_fd);
   
  //back to initial terminal configuration
  tcsetattr(STDOUT_FILENO,TCSANOW,&oldtio);
  tcsetattr(STDOUT_FILENO, TCSAFLUSH, &oldtio);

  return(0);
}

