#include <stdio.h>
#include <string.h>
#include <stdint.h>
static unsigned char mem[0x20000], pat[0x16fda*2]; static unsigned char *pat_mem = pat;
#define STATE ((unsigned)0)
#include "test_body.c"
int main(void){
  int fail=0; s16 locks[112]; for(int i=0;i<112;i++) locks[i]=-1;
  locks[5]=0x3000; locks[101]=3<<8; locks[104]=50<<8; /* MOD=3, LEN=50 */
  u32 off=1*PAT_STRIDE+2*TRK_SIZE; u8*real=pat+off; real[0x20b]=1; real[0x20f]=14; real[0x2d8]=77;
  s16*r=arpl_apply(2,locks);
  if(r==locks||r[101]!=-1||r[104]!=-1||r[5]!=0x3000||locks[101]!=(3<<8)) {puts("apply strip FAIL");fail=1;}
  u8*b=arpl_base(off);
  if(b==real||b[0x20b]!=3||b[0x20f]!=50||b[0x2d8]!=77) {puts("base FAIL");fail=1;}
  if(arpl_base(1*PAT_STRIDE+3*TRK_SIZE)!=pat+1*PAT_STRIDE+3*TRK_SIZE){puts("other track FAIL");fail=1;}
  for(int i=0;i<112;i++) locks[i]=-1;
  if(arpl_apply(2,locks)!=locks){puts("nolock FAIL");fail=1;}
  if(arpl_base(off)!=real){puts("revert FAIL");fail=1;}
  if(arpl_apply(9,locks)!=locks){puts("track>5 FAIL");fail=1;}
  puts(fail?"FAILED":"all passed"); return fail;
}
