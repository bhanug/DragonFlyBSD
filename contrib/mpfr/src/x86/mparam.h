/* Various Thresholds of MPFR, not exported.  -*- mode: C -*-

Copyright 2005, 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

/* Generated by MPFR's tuneup.c, 2011-07-31, gcc 4.4.2 */
/* contributed by Jim Cloos <cloos at jhcloos dot com> with GMP 5.0.2 on a
   Pentium3-M, where __i386, __i686, __pentiumpro are defined */

#define MPFR_MULHIGH_TAB  \
 -1,0,-1,-1,-1,-1,-1,-1,-1,7,8,9,10,0,0,0, \
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
 0,0,0,0,0,0,0,0,0,0,28,30,30,30,30,32, \
 32,32,34,34,34,32,34,34,34,36,36,36,36,40,40,40, \
 42,44,48,48,48,48,48,48,48,48,48,52,52,52,52,52, \
 52,52,56,56,60,56,60,60,60,60,60,64,64,64,64,64, \
 64,64,64,64,64,64,68,64,64,68,68,68,68,72,72,81, \
 81,80,81,81,87,87,87,87,87,87,87,87,87,87,93,87, \
 93,93,93,93,93,93,93,93,99,99,93,93,93,92,93,99, \
 99,99,99,99,99,99,99,99,105,105,99,105,105,104,105,105, \
 105,105,111,117,117,117,117,117,117,117,117,117,117,117,117,117, \
 123,123,123,123,141,141,141,141,141,141,141,141,141,141,141,141, \
 141,141,141,141,141,141,141,141,141,153,153,153,153,153,153,153, \
 153,153,153,153,153,165,165,165,153,165,165,165,165,165,165,165, \
 165,165,165,165,165,177,177,165,177,177,177,177,165,177,177,177, \
 177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, \
 177,177,189,204,189,204,204,204,204,204,204,189,204,189,204,204, \
 204,204,204,204,204,204,204,204,203,204,204,204,204,204,204,204, \
 220,204,220,220,220,220,220,220,220,220,220,220,220,220,220,220, \
 236,220,236,236,236,236,236,235,236,235,236,236,236,236,235,236, \
 236,236,236,236,236,236,236,236,236,252,252,252,252,252,252,252, \
 252,252,252,252,252,252,252,252,282,282,282,282,282,282,282,282, \
 282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282, \
 282,282,282,282,282,282,282,306,306,282,306,306,306,306,306,306, \
 306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306, \
 306,306,306,306,306,330,330,330,329,330,330,330,330,330,330,330, \
 330,330,330,330,330,330,330,330,330,330,330,330,330,330,354,354, \
 354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,354, \
 354,354,354,354,354,354,354,354,354,378,378,378,378,378,378,378, \
 378,377,378,378,378,378,378,378,378,378,378,378,378,378,378,378, \
 408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408, \
 408,408,408,408,408,408,407,408,407,408,408,407,408,408,408,408, \
 408,408,439,408,440,440,439,440,440,440,440,439,440,440,439,440, \
 439,440,439,440,439,439,440,440,440,440,439,440,439,440,439,440, \
 440,440,439,439,440,440,472,440,472,439,440,439,440,440,440,472, \
 472,472,469,471,471,472,471,471,472,471,470,470,472,472,471,472, \
 472,504,504,463,471,455,438,501,462,497,501,503,459,471,451,467, \
 448,501,503,485,504,453,469,470,503,472,496,496,480,467,504,460, \
 504,488,488,532,535,497,480,470,504,497,504,492,497,498,536,468, \
 504,510,519,528,500,500,495,504,504,504,504,534,504,504,498,504, \
 504,511,504,504,504,503,504,563,504,511,504,504,504,504,504,504, \
 504,504,533,504,536,536,536,533,536,536,536,536,536,536,536,536, \
 536,536,536,536,536,536,536,536,536,536,536,536,536,536,566,536, \
 536,536,536,568,568,568,568,568,568,568,568,568,568,568,568,568, \
 568,568,568,568,568,568,568,568,568,568,568,568,568,568,592,592, \
 592,592,592,592,592,592,592,592,640,592,640,592,640,639,640,640, \
 640,640,640,639,640,639,640,640,640,639,640,640,640,640,640,640, \
 640,640,640,640,640,640,640,640,640,640,640,640,640,640,640,640, \
 639,640,639,639,688,688,688,639,640,639,640,640,640,687,688,688, \
 688,687,687,688,688,688,688,688,688,688,688,687,688,736,688,688, \
 688,687,636,688,688,688,688,688,688,688,688,688,688,688,688,688, \
 688,688,688,688,688,688,688,688,688,688,687,688,688,688,688,688, \
 688,688,688,688,688,688,688,688,688,688,688,688,736,736,736,736, \
 736,736,736,736,736,736,736,736,688,735,736,736,736,736,736,736, \
 736,736,736,736,736,736,736,736,736,736,736,735,736,735,736,736, \
 736,735,736,736,736,736,736,736,736,736,736,736,736,736,736,736, \
 736,736,736,736,736,736,736,735,736,736,736,736,736,736,736,736, \
 736,736,736,736,784,736,784,784,736,736,736,736,736,736,736,736, \
 736,736,736,736,736,736,736,736,736,736,736,736,736,736,736,736, \
 784,736,784,784,784,784,784,784,736,784,736,784,784,784,784,736, \
 784,783,784,784,784,784,784,784,784,784,784,784,784,784,784,784, \
 784,784,784,784,784,784,784,784,784,784,784,832,784,784,784,784, \
 832,784,832,831,832,831,832,832,832,832,832,784,784,784,784,784, \
 832,784,832,784,832,832,832,832,832,832,832,832,832,832,832,832 \
  
#define MPFR_SQRHIGH_TAB  \
 -1,0,0,-1,-1,-1,-1,5,6,6,7,7,8,8,9,9, \
 10,11,12,11,12,13,14,13,14,15,16,15,16,16,17,17, \
 18,19,19,19,20,20,21,22,23,22,23,24,25,24,25,26, \
 27,26,27,27,28,28,29,29,30,31,32,31,32,32,33,34, \
 35,34,35,36,36,38,38,38,39,40,40,40,41,41,41,41, \
 42,43,44,44,48,48,48,48,48,50,52,50,52,52,54,54, \
 56,56,58,54,54,54,56,56,56,56,58,58,60,60,62,64, \
 64,64,62,62,64,64,64,64,64,66,66,68,68,70,68,72, \
 70,72,74,72,74,72,72,72,74,74,76,72,72,72,74,74, \
 76,76,76,76,78,78,78,80,80,80,80,80,80,80,87,90, \
 87,90,90,93,93,90,90,90,90,93,93,93,90,96,96,93, \
 96,96,96,96,96,93,99,96,102,99,99,99,96,102,102,102, \
 108,102,105,105,105,108,111,111,108,111,111,111,111,111,111,117, \
 114,117,114,117,117,120,120,120,120,120,120,120,123,126,126,126, \
 126,129,129,129,129,129,132,132,132,135,135,135,141,141,141,141, \
 141,141,148,148,148,152,148,152,152,148,152,156,156,156,156,156, \
 160,135,141,152,141,141,141,141,141,141,141,141,141,141,141,141, \
 141,141,152,141,156,152,156,156,160,160,160,152,160,156,156,156, \
 160,160,156,168,160,168,164,164,168,160,168,176,172,168,168,172, \
 172,172,176,176,188,176,176,172,188,188,188,188,188,187,188,188, \
 188,188,188,188,196,188,196,196,196,196,196,204,204,204,204,204, \
 204,204,204,212,212,212,212,212,212,212,212,220,220,220,220,220, \
 220,220,220,219,220,220,220,228,220,228,220,220,228,228,220,220, \
 220,236,236,228,228,236,228,236,228,244,244,228,244,244,236,236, \
 258,236,258,258,258,258,258,258,258,264,258,264,264,264,264,264, \
 264,264,264,264,264,258,264,258,264,282,258,264,282,282,282,282, \
 282,264,264,282,282,276,264,264,282,294,282,282,282,282,282,282, \
 282,282,282,282,282,282,282,282,282,282,282,282,294,282,282,282, \
 282,282,282,306,282,282,306,306,282,306,306,306,306,318,306,318, \
 318,318,318,318,318,318,318,318,318,318,318,330,330,318,318,318, \
 330,318,318,330,330,318,318,318,318,318,318,330,342,330,330,330, \
 330,330,330,330,330,330,330,330,330,318,330,318,318,342,342,318, \
 342,318,318,342,318,330,330,330,330,330,318,330,330,318,330,318, \
 342,330,342,342,342,342,342,342,342,330,342,342,330,342,342,342, \
 342,354,342,342,342,342,342,342,342,354,342,342,354,354,354,342, \
 354,354,354,354,354,354,354,378,366,366,366,366,378,366,366,366, \
 378,408,366,378,378,378,378,378,378,378,378,378,378,424,424,424, \
 424,424,424,424,424,408,408,408,408,408,408,408,408,408,408,408, \
 408,408,424,424,408,424,408,424,424,408,408,424,424,424,424,424, \
 424,424,424,424,424,424,408,424,424,424,424,424,440,424,440,424, \
 424,440,424,424,424,424,440,440,440,440,424,424,440,440,424,424, \
 424,424,440,424,440,456,456,424,440,440,440,440,456,424,424,424, \
 440,440,456,440,472,456,440,440,472,440,456,440,440,440,456,440, \
 440,440,456,456,456,472,471,472,456,472,440,472,488,488,455,455, \
 456,456,488,456,472,488,504,472,456,504,488,488,456,472,472,471, \
 472,502,472,472,496,488,487,503,456,504,504,503,504,456,456,487, \
 488,488,504,488,487,504,503,503,504,504,503,503,503,504,502,503, \
 502,504,519,501,504,551,502,520,520,488,504,479,517,503,488,463, \
 472,456,504,503,487,519,552,534,485,480,486,502,499,552,504,534, \
 486,440,504,511,512,533,516,472,536,567,534,494,519,440,542,534, \
 528,592,468,536,516,485,472,583,550,509,537,471,493,533,526,446, \
 558,498,499,495,487,571,500,544,471,583,545,530,470,566,520,536, \
 558,463,504,544,534,494,520,542,522,520,552,520,514,483,434,607, \
 566,538,536,552,486,607,623,606,520,453,588,476,614,497,472,504, \
 504,566,552,504,504,504,504,504,504,504,504,504,504,552,520,504, \
 520,520,520,504,520,520,504,520,536,520,520,536,536,536,536,536, \
 536,536,536,586,536,536,520,555,504,565,520,552,536,552,552,552, \
 552,552,552,552,552,552,552,552,552,552,552,552,552,552,568,552, \
 568,552,568,568,568,568,559,560,568,552,552,568,568,552,568,584, \
 552,552,552,552,552,552,568,568,552,568,568,552,568,568,552,552, \
 584,568,568,568,568,568,568,568,568,568,552,568,584,584,568,552, \
 584,584,584,584,584,584,584,568,568,568,568,616,568,568,616,568, \
 600,584,600,600,600,632,616,600,584,616,616,632,632,584,632,616, \
 616,616,616,616,616,632,632,616,616,632,632,616,616,632,616,616 \
  
#define MPFR_DIVHIGH_TAB  \
 0,1,2,3,4,5,6,7,8,7,10,9,10,11,12,13, \
 14,15,16,17,18,16,14,16,16,19,19,18,20,19,20,18, \
 24,19,20,23,24,21,21,23,27,24,28,29,28,28,28,28, \
 31,31,30,28,34,29,32,32,33,34,35,31,34,38,38,34, \
 35,36,38,38,38,40,41,43,39,42,43,42,42,43,44,43, \
 45,48,49,48,51,50,50,48,50,48,51,50,51,52,53,50, \
 55,51,53,54,56,59,56,59,58,55,57,59,59,63,64,63, \
 63,64,64,59,64,64,67,64,63,67,67,71,67,67,71,66, \
 75,75,75,75,73,75,73,75,75,75,73,75,84,73,74,73, \
 76,76,75,81,76,79,81,84,81,83,84,84,84,84,88,86, \
 83,88,96,94,96,96,96,96,96,96,96,95,96,96,96,104, \
 96,100,100,104,104,104,96,96,96,104,100,96,102,96,104,104, \
 102,104,100,104,112,104,104,112,112,112,104,112,120,112,112,112, \
 112,120,110,118,118,118,119,112,120,118,120,112,118,127,128,128, \
 128,128,128,128,128,128,128,128,128,128,128,128,128,136,128,136, \
 128,128,128,128,128,128,128,128,127,128,128,128,128,128,130,133, \
 134,136,136,135,136,136,150,136,136,138,148,140,136,143,148,150, \
 150,150,150,148,150,148,148,150,156,149,168,150,160,161,168,168, \
 168,168,168,162,168,162,167,168,168,168,168,168,162,174,168,168, \
 174,174,184,168,168,176,162,168,192,174,168,168,173,174,186,174, \
 174,168,192,174,168,174,168,168,186,192,192,180,174,176,192,192, \
 185,192,192,180,192,192,186,192,192,192,192,192,184,192,208,208, \
 208,208,190,192,190,192,192,191,186,192,192,208,192,192,192,192, \
 192,192,196,204,204,192,204,192,208,192,191,192,208,208,198,208, \
 205,220,197,218,222,208,216,199,207,220,206,224,214,204,208,219, \
 221,209,227,207,221,216,208,236,222,244,225,208,208,212,224,222, \
 229,230,225,221,222,256,227,240,221,224,240,216,240,231,224,263, \
 246,232,240,240,240,240,239,240,240,234,234,240,234,238,240,240, \
 240,240,256,256,240,256,240,240,240,234,256,240,240,256,234,256, \
 256,239,272,244,256,255,256,240,256,240,256,255,240,255,256,256, \
 256,254,254,256,244,256,256,256,256,256,256,256,256,253,272,255, \
 256,256,252,256,256,256,256,256,256,272,256,256,256,256,257,272, \
 272,272,272,272,264,270,272,288,272,280,282,282,280,280,270,282, \
 282,282,288,282,288,282,282,300,286,288,300,282,282,288,300,312, \
 300,298,306,300,304,304,304,300,300,300,316,306,300,288,306,311, \
 324,312,320,300,306,336,320,300,324,320,336,336,336,328,336,324, \
 336,336,328,336,322,328,336,336,336,336,312,336,336,336,336,336, \
 336,336,336,335,336,344,348,336,353,349,336,336,348,348,346,336, \
 352,320,336,336,336,336,352,336,336,336,336,336,336,336,336,336, \
 336,336,336,330,336,336,336,336,334,335,336,336,344,344,348,336, \
 348,336,352,336,336,348,336,348,336,348,352,351,348,336,336,336, \
 352,352,352,348,352,336,336,384,336,336,352,348,336,348,376,352, \
 352,372,384,352,384,352,352,352,348,384,348,349,384,351,384,353, \
 368,352,350,353,378,372,372,372,384,352,384,352,408,384,378,368, \
 383,384,408,356,371,368,372,371,384,376,384,408,384,407,372,384, \
 408,369,406,383,377,384,394,382,370,416,408,378,369,372,381,400, \
 378,408,376,402,417,408,376,384,382,384,384,408,384,384,384,408, \
 384,416,408,408,416,384,384,384,408,384,384,384,396,408,408,408, \
 408,408,408,406,408,408,416,416,408,408,408,408,408,416,408,416, \
 408,405,416,408,408,440,408,416,416,416,440,408,408,408,440,444, \
 408,440,440,408,415,419,440,440,444,443,436,436,440,447,448,440, \
 440,416,440,439,440,444,440,444,468,448,440,416,440,416,465,465, \
 468,436,468,440,472,439,440,468,472,448,465,468,464,439,440,480, \
 480,472,439,468,440,440,468,440,468,468,444,440,448,480,438,440, \
 504,439,440,467,480,468,448,465,466,440,468,464,440,472,448,468, \
 468,469,456,465,448,460,472,472,468,469,465,480,480,468,472,512, \
 468,464,465,469,480,468,504,466,468,480,480,480,468,512,504,469, \
 466,466,472,468,480,480,512,469,472,480,480,480,504,477,500,466, \
 504,468,480,512,504,512,504,480,480,511,512,510,492,511,503,513, \
 503,502,504,512,504,512,504,512,504,512,504,512,513,503,512,504, \
 513,504,512,512,510,501,513,503,512,513,498,504,512,510,502,512, \
 501,500,552,497,516,511,503,513,497,511,502,509,564,509,504,512, \
 503,504,512,504,504,500,512,512,512,511,512,504,557,504,564,564, \
 512,564,514,512,516,512,560,564,560,564,564,512,564,564,564,560 \
  
#define MPFR_MUL_THRESHOLD 15 /* limbs */
#define MPFR_SQR_THRESHOLD 14 /* limbs */
#define MPFR_DIV_THRESHOLD 27 /* limbs */
#define MPFR_EXP_2_THRESHOLD 562 /* bits */
#define MPFR_EXP_THRESHOLD 9671 /* bits */
#define MPFR_SINCOS_THRESHOLD 30620 /* bits */
#define MPFR_AI_THRESHOLD1 -28021 /* threshold for negative input of mpfr_ai */
#define MPFR_AI_THRESHOLD2 2991
#define MPFR_AI_THRESHOLD3 37474
/* Tuneup completed successfully, took 6469 seconds */
