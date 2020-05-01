#pragma once
#include <board_map.h>

#define USB_GOTGCTL        (reg32_t)(USB_BASE + 0x0000) //  RW 20 0x000f0f03
#define USB_GOTGINT        (reg32_t)(USB_BASE + 0x0004) //  RW 20 0x000e0304
#define USB_GAHBCFG        (reg32_t)(USB_BASE + 0x0008) //  RW 9 0x000001bf
#define USB_GUSBCFG        (reg32_t)(USB_BASE + 0x000c) //  RW 32 0xe3ffbfff
#define USB_GRSTCTL        (reg32_t)(USB_BASE + 0x0010) //  RW 32 0xc00007ff
#define USB_GINTSTS        (reg32_t)(USB_BASE + 0x0014) //  RW 32 0xffffffff
#define USB_GINTMSK        (reg32_t)(USB_BASE + 0x0018) //  RW 32 0xf77effff
#define USB_GRXSTSR        (reg32_t)(USB_BASE + 0x001c) //  RW 32 0xffffffff
#define USB_GRXSTSP        (reg32_t)(USB_BASE + 0x0020) //  RW 25 0x01ffffff
#define USB_GRXFSIZ        (reg32_t)(USB_BASE + 0x0024) //  RW 16 0x0000ffff
#define USB_GNPTXFSIZ      (reg32_t)(USB_BASE + 0x0028) //  RW 32 0xffffffff
#define USB_GNPTXSTS       (reg32_t)(USB_BASE + 0x002c) //  RW 31 0x7fffffff
#define USB_GI2CCTL        (reg32_t)(USB_BASE + 0x0030) //  RW 32 0xdfffffff
#define USB_GPVNDCTL       (reg32_t)(USB_BASE + 0x0034) //  RW 32 0x8e7f3fff
#define USB_GGPIO          (reg32_t)(USB_BASE + 0x0038) //  RW 32 0xffffffff
#define USB_GUID           (reg32_t)(USB_BASE + 0x003c) //  RW 32 0xffffffff
#define USB_GSNPSID        (reg32_t)(USB_BASE + 0x0040) //  RW 32 0xffffffff

#define USB_GHWCFG1        (reg32_t)(USB_BASE + 0x0044) //  RW 32 0xffffffff
#define USB_GHWCFG2        (reg32_t)(USB_BASE + 0x0048) //  RW 31 0x7fcfffff
#define USB_GHWCFG3        (reg32_t)(USB_BASE + 0x004c) //  RW 32 0xffff0fff
#define USB_GHWCFG4        (reg32_t)(USB_BASE + 0x0050) //  RW 32 0xffffc03f

#define USB_GLPMCFG        (reg32_t)(USB_BASE + 0x0054) //  RW 32 0xffffffff
#define USB_GAXIDEV        (reg32_t)(USB_BASE + 0x0054) //  RW 32 0xffffffff
#define USB_GMDIOCSR       (reg32_t)(USB_BASE + 0x0080) //  RW 16 0x0000ffff
#define USB_GMDIOGEN       (reg32_t)(USB_BASE + 0x0084) //  RW 32 0xffffffff
#define USB_GVBUSDRV       (reg32_t)(USB_BASE + 0x0088) //  RW 16 0x0000ffff
#define USB_HPTXFSIZ       (reg32_t)(USB_BASE + 0x0100) //  RW 32 0xffffffff

#define USB_DIEPTXF1       (reg32_t)(USB_BASE + 0x0104) //  RW 32 0xffffffff
#define USB_DPTXFSIZ1      (reg32_t)(USB_BASE + 0x0104) //  RW 32 0xffffffff

#define USB_DPTXFSIZ2      (reg32_t)(USB_BASE + 0x0108) //  RW 32 0xffffffff
#define USB_DIEPTXF2       (reg32_t)(USB_BASE + 0x0108) //  RW 32 0xffffffff

#define USB_DIEPTXF3       (reg32_t)(USB_BASE + 0x010c) //  RW 32 0xffffffff
#define USB_DPTXFSIZ3      (reg32_t)(USB_BASE + 0x010c) //  RW 32 0xffffffff

#define USB_DIEPTXF4       (reg32_t)(USB_BASE + 0x0110) //  RW 32 0xffffffff
#define USB_DPTXFSIZ4      (reg32_t)(USB_BASE + 0x0110) //  RW 32 0xffffffff

#define USB_DPTXFSIZ5      (reg32_t)(USB_BASE + 0x0114) //  RW 32 0xffffffff
#define USB_DIEPTXF5       (reg32_t)(USB_BASE + 0x0114) //  RW 32 0xffffffff

#define USB_DIEPTXF6       (reg32_t)(USB_BASE + 0x0118) //  RW 32 0xffffffff
#define USB_DPTXFSIZ6      (reg32_t)(USB_BASE + 0x0118) //  RW 32 0xffffffff

#define USB_DIEPTXF7       (reg32_t)(USB_BASE + 0x011c) //  RW 32 0xffffffff
#define USB_DPTXFSIZ7      (reg32_t)(USB_BASE + 0x011c) //  RW 32 0xffffffff

#define USB_DIEPTXF8       (reg32_t)(USB_BASE + 0x0120) //  RW 32 0xffffffff
#define USB_DPTXFSIZ8      (reg32_t)(USB_BASE + 0x0120) //  RW 32 0xffffffff

#define USB_DIEPTXF9       (reg32_t)(USB_BASE + 0x0124) //  RW 32 0xffffffff
#define USB_DPTXFSIZ9      (reg32_t)(USB_BASE + 0x0124) //  RW 32 0xffffffff

#define USB_DPTXFSIZ10     (reg32_t)(USB_BASE + 0x0128) //  RW 32 0xffffffff
#define USB_DIEPTXF10      (reg32_t)(USB_BASE + 0x0128) //  RW 32 0xffffffff

#define USB_DIEPTXF11      (reg32_t)(USB_BASE + 0x012c) //  RW 32 0xffffffff
#define USB_DPTXFSIZ11     (reg32_t)(USB_BASE + 0x012c) //  RW 32 0xffffffff

#define USB_DIEPTXF12      (reg32_t)(USB_BASE + 0x0130) //  RW 32 0xffffffff
#define USB_DPTXFSIZ12     (reg32_t)(USB_BASE + 0x0130) //  RW 32 0xffffffff

#define USB_DPTXFSIZ13     (reg32_t)(USB_BASE + 0x0134) //  RW 32 0xffffffff
#define USB_DIEPTXF13      (reg32_t)(USB_BASE + 0x0134) //  RW 32 0xffffffff

#define USB_DPTXFSIZ14     (reg32_t)(USB_BASE + 0x0138) //  RW 32 0xffffffff
#define USB_DIEPTXF14      (reg32_t)(USB_BASE + 0x0138) //  RW 32 0xffffffff

#define USB_DIEPTXF15      (reg32_t)(USB_BASE + 0x013c) //  RW 32 0xffffffff
#define USB_DPTXFSIZ15     (reg32_t)(USB_BASE + 0x013c) //  RW 32 0xffffffff

#define USB_HCFG           (reg32_t)(USB_BASE + 0x0400) //  RW 3 0x00000007
#define USB_HFIR           (reg32_t)(USB_BASE + 0x0404) //  RW 16 0x0000ffff
#define USB_HFNUM          (reg32_t)(USB_BASE + 0x0408) //  RW 32 0xffffffff
#define USB_HPTXSTS        (reg32_t)(USB_BASE + 0x0410) //  RW 32 0xffffffff
#define USB_HAINT          (reg32_t)(USB_BASE + 0x0414) //  RW 32 0xffffffff
#define USB_HAINTMSK       (reg32_t)(USB_BASE + 0x0418) //  RW 32 0xffffffff
#define USB_HPRT           (reg32_t)(USB_BASE + 0x0440) //  RW 19 0x0007fdff

#define USB_HCCHAR0        (reg32_t)(USB_BASE + 0x0500) //  RW 32 0xffffffff
#define USB_HCSPLT0        (reg32_t)(USB_BASE + 0x0504) //  RW 32 0xffffffff
#define USB_HCINT0         (reg32_t)(USB_BASE + 0x0508) //  RW 32 0xffffffff
#define USB_HCINTMSK0      (reg32_t)(USB_BASE + 0x050c) //  RW 32 0xffffffff
#define USB_HCTSIZ0        (reg32_t)(USB_BASE + 0x0510) //  RW 32 0xffffffff
#define USB_HCDMA0         (reg32_t)(USB_BASE + 0x0514) //  RW 32 0xffffffff

#define USB_HCCHAR1        (reg32_t)(USB_BASE + 0x0520) //  RW 32 0xffffffff
#define USB_HCSPLT1        (reg32_t)(USB_BASE + 0x0524) //  RW 32 0xffffffff
#define USB_HCINT1         (reg32_t)(USB_BASE + 0x0528) //  RW 32 0xffffffff
#define USB_HCINTMSK1      (reg32_t)(USB_BASE + 0x052c) //  RW 32 0xffffffff
#define USB_HCTSIZ1        (reg32_t)(USB_BASE + 0x0530) //  RW 32 0xffffffff
#define USB_HCDMA1         (reg32_t)(USB_BASE + 0x0534) //  RW 32 0xffffffff

#define USB_HCCHAR2        (reg32_t)(USB_BASE + 0x0540) //  RW 32 0xffffffff
#define USB_HCSPLT2        (reg32_t)(USB_BASE + 0x0544) //  RW 32 0xffffffff
#define USB_HCINT2         (reg32_t)(USB_BASE + 0x0548) //  RW 32 0xffffffff
#define USB_HCINTMSK2      (reg32_t)(USB_BASE + 0x054c) //  RW 32 0xffffffff
#define USB_HCTSIZ2        (reg32_t)(USB_BASE + 0x0550) //  RW 32 0xffffffff
#define USB_HCDMA2         (reg32_t)(USB_BASE + 0x0554) //  RW 32 0xffffffff

#define USB_HCCHAR3        (reg32_t)(USB_BASE + 0x0560) //  RW 32 0xffffffff
#define USB_HCSPLT3        (reg32_t)(USB_BASE + 0x0564) //  RW 32 0xffffffff
#define USB_HCINT3         (reg32_t)(USB_BASE + 0x0568) //  RW 32 0xffffffff
#define USB_HCINTMSK3      (reg32_t)(USB_BASE + 0x056c) //  RW 32 0xffffffff
#define USB_HCTSIZ3        (reg32_t)(USB_BASE + 0x0570) //  RW 32 0xffffffff
#define USB_HCDMA3         (reg32_t)(USB_BASE + 0x0574) //  RW 32 0xffffffff

#define USB_HCCHAR4        (reg32_t)(USB_BASE + 0x0580) //  RW 32 0xffffffff
#define USB_HCSPLT4        (reg32_t)(USB_BASE + 0x0584) //  RW 32 0xffffffff
#define USB_HCINT4         (reg32_t)(USB_BASE + 0x0588) //  RW 32 0xffffffff
#define USB_HCINTMSK4      (reg32_t)(USB_BASE + 0x058c) //  RW 32 0xffffffff
#define USB_HCTSIZ4        (reg32_t)(USB_BASE + 0x0590) //  RW 32 0xffffffff
#define USB_HCDMA4         (reg32_t)(USB_BASE + 0x0594) //  RW 32 0xffffffff

#define USB_HCCHAR5        (reg32_t)(USB_BASE + 0x05a0) //  RW 32 0xffffffff
#define USB_HCSPLT5        (reg32_t)(USB_BASE + 0x05a4) //  RW 32 0xffffffff
#define USB_HCINT5         (reg32_t)(USB_BASE + 0x05a8) //  RW 32 0xffffffff
#define USB_HCINTMSK5      (reg32_t)(USB_BASE + 0x05ac) //  RW 32 0xffffffff
#define USB_HCTSIZ5        (reg32_t)(USB_BASE + 0x05b0) //  RW 32 0xffffffff
#define USB_HCDMA5         (reg32_t)(USB_BASE + 0x05b4) //  RW 32 0xffffffff

#define USB_HCCHAR6        (reg32_t)(USB_BASE + 0x05c0) //  RW 32 0xffffffff
#define USB_HCSPLT6        (reg32_t)(USB_BASE + 0x05c4) //  RW 32 0xffffffff
#define USB_HCINT6         (reg32_t)(USB_BASE + 0x05c8) //  RW 32 0xffffffff
#define USB_HCINTMSK6      (reg32_t)(USB_BASE + 0x05cc) //  RW 32 0xffffffff
#define USB_HCTSIZ6        (reg32_t)(USB_BASE + 0x05d0) //  RW 32 0xffffffff
#define USB_HCDMA6         (reg32_t)(USB_BASE + 0x05d4) //  RW 32 0xffffffff

#define USB_HCCHAR7        (reg32_t)(USB_BASE + 0x05e0) //  RW 32 0xffffffff
#define USB_HCSPLT7        (reg32_t)(USB_BASE + 0x05e4) //  RW 32 0xffffffff
#define USB_HCINT7         (reg32_t)(USB_BASE + 0x05e8) //  RW 32 0xffffffff
#define USB_HCINTMSK7      (reg32_t)(USB_BASE + 0x05ec) //  RW 32 0xffffffff
#define USB_HCTSIZ7        (reg32_t)(USB_BASE + 0x05f0) //  RW 32 0xffffffff
#define USB_HCDMA7         (reg32_t)(USB_BASE + 0x05f4) //  RW 32 0xffffffff

#define USB_DCFG           (reg32_t)(USB_BASE + 0x0800) //  RW 26 0x03fc1ff7
#define USB_DCTL           (reg32_t)(USB_BASE + 0x0804) //  RW 16 0x0000efff
#define USB_DSTS           (reg32_t)(USB_BASE + 0x0808) //  RW 22 0x003fff0f
#define USB_DIEPMSK        (reg32_t)(USB_BASE + 0x0810) //  RW 32 0xffffffff
#define USB_DOEPMSK        (reg32_t)(USB_BASE + 0x0814) //  RW 32 0xffffffff
#define USB_DAINT          (reg32_t)(USB_BASE + 0x0818) //  RW 32 0xffffffff
#define USB_DAINTMSK       (reg32_t)(USB_BASE + 0x081c) //  RW 32 0xffffffff
#define USB_DTKNQR1        (reg32_t)(USB_BASE + 0x0820) //  RW 32 0xffffffff
#define USB_DTKNQR2        (reg32_t)(USB_BASE + 0x0824) //  RW 32 0xffffffff
#define USB_DVBUSDIS       (reg32_t)(USB_BASE + 0x0828) //  RW 16 0x0000ffff
#define USB_DVBUSPULSE     (reg32_t)(USB_BASE + 0x082c) //  RW 12 0x00000fff
#define USB_DTKNQR3        (reg32_t)(USB_BASE + 0x0830) //  RW 32 0xffffffff
#define USB_DTHRCTL        (reg32_t)(USB_BASE + 0x0830) //  RW 28 0x0fff0fff
#define USB_DIEPEMPMSK     (reg32_t)(USB_BASE + 0x0834) //  RW 16 0x0000ffff
#define USB_DTKNQR4        (reg32_t)(USB_BASE + 0x0834) //  RW 32 0xffffffff

#define USB_DIEPCTL0       (reg32_t)(USB_BASE + 0x0900) //  RW 32 0xffffffff
#define USB_DIEPINT0       (reg32_t)(USB_BASE + 0x0908) //  RW 32 0xffffffff
#define USB_DIEPTSIZ0      (reg32_t)(USB_BASE + 0x0910) //  RW 32 0xffffffff
#define USB_DIEPDMA0       (reg32_t)(USB_BASE + 0x0914) //  RW 32 0xffffffff
#define USB_DTXFSTS0       (reg32_t)(USB_BASE + 0x0918) //  RW 32 0xffffffff
#define USB_DIEPDMAB0      (reg32_t)(USB_BASE + 0x0918) //  RW 32 0xffffffff

#define USB_DIEPCTL1       (reg32_t)(USB_BASE + 0x0920) //  RW 32 0xffffffff
#define USB_DIEPINT1       (reg32_t)(USB_BASE + 0x0928) //  RW 32 0xffffffff
#define USB_DIEPTSIZ1      (reg32_t)(USB_BASE + 0x0930) //  RW 32 0xffffffff
#define USB_DIEPDMA1       (reg32_t)(USB_BASE + 0x0934) //  RW 32 0xffffffff
#define USB_DTXFSTS1       (reg32_t)(USB_BASE + 0x0938) //  RW 32 0xffffffff
#define USB_DIEPDMAB1      (reg32_t)(USB_BASE + 0x0938) //  RW 32 0xffffffff

#define USB_DIEPCTL2       (reg32_t)(USB_BASE + 0x0940) //  RW 32 0xffffffff
#define USB_DIEPINT2       (reg32_t)(USB_BASE + 0x0948) //  RW 32 0xffffffff
#define USB_DIEPTSIZ2      (reg32_t)(USB_BASE + 0x0950) //  RW 32 0xffffffff
#define USB_DIEPDMA2       (reg32_t)(USB_BASE + 0x0954) //  RW 32 0xffffffff
#define USB_DTXFSTS2       (reg32_t)(USB_BASE + 0x0958) //  RW 32 0xffffffff
#define USB_DIEPDMAB2      (reg32_t)(USB_BASE + 0x0958) //  RW 32 0xffffffff

#define USB_DIEPCTL3       (reg32_t)(USB_BASE + 0x0960) //  RW 32 0xffffffff
#define USB_DIEPINT3       (reg32_t)(USB_BASE + 0x0968) //  RW 32 0xffffffff
#define USB_DIEPTSIZ3      (reg32_t)(USB_BASE + 0x0970) //  RW 32 0xffffffff
#define USB_DIEPDMA3       (reg32_t)(USB_BASE + 0x0974) //  RW 32 0xffffffff
#define USB_DTXFSTS3       (reg32_t)(USB_BASE + 0x0978) //  RW 32 0xffffffff
#define USB_DIEPDMAB3      (reg32_t)(USB_BASE + 0x0978) //  RW 32 0xffffffff

#define USB_DIEPCTL4       (reg32_t)(USB_BASE + 0x0980) //  RW 32 0xffffffff
#define USB_DIEPINT4       (reg32_t)(USB_BASE + 0x0988) //  RW 32 0xffffffff
#define USB_DIEPTSIZ4      (reg32_t)(USB_BASE + 0x0990) //  RW 32 0xffffffff
#define USB_DIEPDMA4       (reg32_t)(USB_BASE + 0x0994) //  RW 32 0xffffffff
#define USB_DTXFSTS4       (reg32_t)(USB_BASE + 0x0998) //  RW 32 0xffffffff
#define USB_DIEPDMAB4      (reg32_t)(USB_BASE + 0x0998) //  RW 32 0xffffffff

#define USB_DIEPCTL5       (reg32_t)(USB_BASE + 0x09a0) //  RW 32 0xffffffff
#define USB_DIEPINT5       (reg32_t)(USB_BASE + 0x09a8) //  RW 32 0xffffffff
#define USB_DIEPTSIZ5      (reg32_t)(USB_BASE + 0x09b0) //  RW 32 0xffffffff
#define USB_DIEPDMA5       (reg32_t)(USB_BASE + 0x09b4) //  RW 32 0xffffffff
#define USB_DIEPDMAB5      (reg32_t)(USB_BASE + 0x09b8) //  RW 32 0xffffffff
#define USB_DTXFSTS5       (reg32_t)(USB_BASE + 0x09b8) //  RW 32 0xffffffff

#define USB_DIEPCTL6       (reg32_t)(USB_BASE + 0x09c0) //  RW 32 0xffffffff
#define USB_DIEPINT6       (reg32_t)(USB_BASE + 0x09c8) //  RW 32 0xffffffff
#define USB_DIEPTSIZ6      (reg32_t)(USB_BASE + 0x09d0) //  RW 32 0xffffffff
#define USB_DIEPDMA6       (reg32_t)(USB_BASE + 0x09d4) //  RW 32 0xffffffff
#define USB_DIEPDMAB6      (reg32_t)(USB_BASE + 0x09d8) //  RW 32 0xffffffff
#define USB_DTXFSTS6       (reg32_t)(USB_BASE + 0x09d8) //  RW 32 0xffffffff

#define USB_DIEPCTL7       (reg32_t)(USB_BASE + 0x09e0) //  RW 32 0xffffffff
#define USB_DIEPINT7       (reg32_t)(USB_BASE + 0x09e8) //  RW 32 0xffffffff
#define USB_DIEPTSIZ7      (reg32_t)(USB_BASE + 0x09f0) //  RW 32 0xffffffff
#define USB_DIEPDMA7       (reg32_t)(USB_BASE + 0x09f4) //  RW 32 0xffffffff
#define USB_DTXFSTS7       (reg32_t)(USB_BASE + 0x09f8) //  RW 32 0xffffffff
#define USB_DIEPDMAB7      (reg32_t)(USB_BASE + 0x09f8) //  RW 32 0xffffffff

#define USB_DIEPCTL8       (reg32_t)(USB_BASE + 0x0a00) //  RW 32 0xffffffff
#define USB_DIEPINT8       (reg32_t)(USB_BASE + 0x0a08) //  RW 32 0xffffffff
#define USB_DIEPTSIZ8      (reg32_t)(USB_BASE + 0x0a10) //  RW 32 0xffffffff
#define USB_DIEPDMA8       (reg32_t)(USB_BASE + 0x0a14) //  RW 32 0xffffffff
#define USB_DTXFSTS8       (reg32_t)(USB_BASE + 0x0a18) //  RW 32 0xffffffff
#define USB_DIEPDMAB8      (reg32_t)(USB_BASE + 0x0a18) //  RW 32 0xffffffff

#define USB_DIEPCTL9       (reg32_t)(USB_BASE + 0x0a20) //  RW 32 0xffffffff
#define USB_DIEPINT9       (reg32_t)(USB_BASE + 0x0a28) //  RW 32 0xffffffff
#define USB_DIEPTSIZ9      (reg32_t)(USB_BASE + 0x0a30) //  RW 32 0xffffffff
#define USB_DIEPDMA9       (reg32_t)(USB_BASE + 0x0a34) //  RW 32 0xffffffff
#define USB_DTXFSTS9       (reg32_t)(USB_BASE + 0x0a38) //  RW 32 0xffffffff
#define USB_DIEPDMAB9      (reg32_t)(USB_BASE + 0x0a38) //  RW 32 0xffffffff

#define USB_DIEPCTL10      (reg32_t)(USB_BASE + 0x0a40) //  RW 32 0xffffffff
#define USB_DIEPINT10      (reg32_t)(USB_BASE + 0x0a48) //  RW 32 0xffffffff
#define USB_DIEPTSIZ10     (reg32_t)(USB_BASE + 0x0a50) //  RW 32 0xffffffff
#define USB_DIEPDMA10      (reg32_t)(USB_BASE + 0x0a54) //  RW 32 0xffffffff
#define USB_DTXFSTS10      (reg32_t)(USB_BASE + 0x0a58) //  RW 32 0xffffffff
#define USB_DIEPDMAB10     (reg32_t)(USB_BASE + 0x0a58) //  RW 32 0xffffffff

#define USB_DIEPCTL11      (reg32_t)(USB_BASE + 0x0a60) //  RW 32 0xffffffff
#define USB_DIEPINT11      (reg32_t)(USB_BASE + 0x0a68) //  RW 32 0xffffffff
#define USB_DIEPTSIZ11     (reg32_t)(USB_BASE + 0x0a70) //  RW 32 0xffffffff
#define USB_DIEPDMA11      (reg32_t)(USB_BASE + 0x0a74) //  RW 32 0xffffffff
#define USB_DTXFSTS11      (reg32_t)(USB_BASE + 0x0a78) //  RW 32 0xffffffff
#define USB_DIEPDMAB11     (reg32_t)(USB_BASE + 0x0a78) //  RW 32 0xffffffff

#define USB_DIEPCTL12      (reg32_t)(USB_BASE + 0x0a80) //  RW 32 0xffffffff
#define USB_DIEPINT12      (reg32_t)(USB_BASE + 0x0a88) //  RW 32 0xffffffff
#define USB_DIEPTSIZ12     (reg32_t)(USB_BASE + 0x0a90) //  RW 32 0xffffffff
#define USB_DIEPDMA12      (reg32_t)(USB_BASE + 0x0a94) //  RW 32 0xffffffff
#define USB_DTXFSTS12      (reg32_t)(USB_BASE + 0x0a98) //  RW 32 0xffffffff
#define USB_DIEPDMAB12     (reg32_t)(USB_BASE + 0x0a98) //  RW 32 0xffffffff

#define USB_DIEPCTL13      (reg32_t)(USB_BASE + 0x0aa0) //  RW 32 0xffffffff
#define USB_DIEPINT13      (reg32_t)(USB_BASE + 0x0aa8) //  RW 32 0xffffffff
#define USB_DIEPTSIZ13     (reg32_t)(USB_BASE + 0x0ab0) //  RW 32 0xffffffff
#define USB_DIEPDMA13      (reg32_t)(USB_BASE + 0x0ab4) //  RW 32 0xffffffff
#define USB_DTXFSTS13      (reg32_t)(USB_BASE + 0x0ab8) //  RW 32 0xffffffff
#define USB_DIEPDMAB13     (reg32_t)(USB_BASE + 0x0ab8) //  RW 32 0xffffffff

#define USB_DIEPCTL14      (reg32_t)(USB_BASE + 0x0ac0) //  RW 32 0xffffffff
#define USB_DIEPINT14      (reg32_t)(USB_BASE + 0x0ac8) //  RW 32 0xffffffff
#define USB_DIEPTSIZ14     (reg32_t)(USB_BASE + 0x0ad0) //  RW 32 0xffffffff
#define USB_DIEPDMA14      (reg32_t)(USB_BASE + 0x0ad4) //  RW 32 0xffffffff
#define USB_DTXFSTS14      (reg32_t)(USB_BASE + 0x0ad8) //  RW 32 0xffffffff
#define USB_DIEPDMAB14     (reg32_t)(USB_BASE + 0x0ad8) //  RW 32 0xffffffff

#define USB_DIEPCTL15      (reg32_t)(USB_BASE + 0x0ae0) //  RW 32 0xffffffff
#define USB_DIEPINT15      (reg32_t)(USB_BASE + 0x0ae8) //  RW 32 0xffffffff
#define USB_DIEPTSIZ15     (reg32_t)(USB_BASE + 0x0af0) //  RW 32 0xffffffff
#define USB_DIEPDMA15      (reg32_t)(USB_BASE + 0x0af4) //  RW 32 0xffffffff
#define USB_DTXFSTS15      (reg32_t)(USB_BASE + 0x0af8) //  RW 32 0xffffffff
#define USB_DIEPDMAB15     (reg32_t)(USB_BASE + 0x0af8) //  RW 32 0xffffffff

#define USB_DOEPCTL0       (reg32_t)(USB_BASE + 0x0b00) //  RW 32 0xffffffff
#define USB_DOEPINT0       (reg32_t)(USB_BASE + 0x0b08) //  RW 32 0xffffffff
#define USB_DOEPTSIZ0      (reg32_t)(USB_BASE + 0x0b10) //  RW 32 0xffffffff
#define USB_DOEPDMA0       (reg32_t)(USB_BASE + 0x0b14) //  RW 32 0xffffffff
#define USB_DOEPDMAB4      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB2      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB1      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB7      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB3      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB15     (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB11     (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB0      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB14     (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB8      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB10     (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB13     (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB5      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB6      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB9      (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff
#define USB_DOEPDMAB12     (reg32_t)(USB_BASE + 0x0b1c) //  RW 32 0xffffffff

#define USB_DOEPCTL1       (reg32_t)(USB_BASE + 0x0b20) //  RW 32 0xffffffff
#define USB_DOEPINT1       (reg32_t)(USB_BASE + 0x0b28) //  RW 32 0xffffffff
#define USB_DOEPTSIZ1      (reg32_t)(USB_BASE + 0x0b30) //  RW 32 0xffffffff
#define USB_DOEPDMA1       (reg32_t)(USB_BASE + 0x0b34) //  RW 32 0xffffffff

#define USB_DOEPCTL2       (reg32_t)(USB_BASE + 0x0b40) //  RW 32 0xffffffff
#define USB_DOEPINT2       (reg32_t)(USB_BASE + 0x0b48) //  RW 32 0xffffffff
#define USB_DOEPTSIZ2      (reg32_t)(USB_BASE + 0x0b50) //  RW 32 0xffffffff
#define USB_DOEPDMA2       (reg32_t)(USB_BASE + 0x0b54) //  RW 32 0xffffffff

#define USB_DOEPCTL3       (reg32_t)(USB_BASE + 0x0b60) //  RW 32 0xffffffff
#define USB_DOEPINT3       (reg32_t)(USB_BASE + 0x0b68) //  RW 32 0xffffffff
#define USB_DOEPTSIZ3      (reg32_t)(USB_BASE + 0x0b70) //  RW 32 0xffffffff
#define USB_DOEPDMA3       (reg32_t)(USB_BASE + 0x0b74) //  RW 32 0xffffffff

#define USB_DOEPCTL4       (reg32_t)(USB_BASE + 0x0b80) //  RW 32 0xffffffff
#define USB_DOEPINT4       (reg32_t)(USB_BASE + 0x0b88) //  RW 32 0xffffffff
#define USB_DOEPTSIZ4      (reg32_t)(USB_BASE + 0x0b90) //  RW 32 0xffffffff
#define USB_DOEPDMA4       (reg32_t)(USB_BASE + 0x0b94) //  RW 32 0xffffffff

#define USB_DOEPCTL5       (reg32_t)(USB_BASE + 0x0ba0) //  RW 32 0xffffffff
#define USB_DOEPINT5       (reg32_t)(USB_BASE + 0x0ba8) //  RW 32 0xffffffff
#define USB_DOEPTSIZ5      (reg32_t)(USB_BASE + 0x0bb0) //  RW 32 0xffffffff
#define USB_DOEPDMA5       (reg32_t)(USB_BASE + 0x0bb4) //  RW 32 0xffffffff

#define USB_DOEPCTL6       (reg32_t)(USB_BASE + 0x0bc0) //  RW 32 0xffffffff
#define USB_DOEPINT6       (reg32_t)(USB_BASE + 0x0bc8) //  RW 32 0xffffffff
#define USB_DOEPTSIZ6      (reg32_t)(USB_BASE + 0x0bd0) //  RW 32 0xffffffff
#define USB_DOEPDMA6       (reg32_t)(USB_BASE + 0x0bd4) //  RW 32 0xffffffff

#define USB_DOEPCTL7       (reg32_t)(USB_BASE + 0x0be0) //  RW 32 0xffffffff
#define USB_DOEPINT7       (reg32_t)(USB_BASE + 0x0be8) //  RW 32 0xffffffff
#define USB_DOEPTSIZ7      (reg32_t)(USB_BASE + 0x0bf0) //  RW 32 0xffffffff
#define USB_DOEPDMA7       (reg32_t)(USB_BASE + 0x0bf4) //  RW 32 0xffffffff

#define USB_DOEPCTL8       (reg32_t)(USB_BASE + 0x0c00) //  RW 32 0xffffffff
#define USB_DOEPINT8       (reg32_t)(USB_BASE + 0x0c08) //  RW 32 0xffffffff
#define USB_DOEPTSIZ8      (reg32_t)(USB_BASE + 0x0c10) //  RW 32 0xffffffff
#define USB_DOEPDMA8       (reg32_t)(USB_BASE + 0x0c14) //  RW 32 0xffffffff

#define USB_DOEPCTL9       (reg32_t)(USB_BASE + 0x0c20) //  RW 32 0xffffffff
#define USB_DOEPINT9       (reg32_t)(USB_BASE + 0x0c28) //  RW 32 0xffffffff
#define USB_DOEPTSIZ9      (reg32_t)(USB_BASE + 0x0c30) //  RW 32 0xffffffff
#define USB_DOEPDMA9       (reg32_t)(USB_BASE + 0x0c34) //  RW 32 0xffffffff

#define USB_DOEPCTL10      (reg32_t)(USB_BASE + 0x0c40) //  RW 32 0xffffffff
#define USB_DOEPINT10      (reg32_t)(USB_BASE + 0x0c48) //  RW 32 0xffffffff
#define USB_DOEPTSIZ10     (reg32_t)(USB_BASE + 0x0c50) //  RW 32 0xffffffff
#define USB_DOEPDMA10      (reg32_t)(USB_BASE + 0x0c54) //  RW 32 0xffffffff

#define USB_DOEPCTL11      (reg32_t)(USB_BASE + 0x0c60) //  RW 32 0xffffffff
#define USB_DOEPINT11      (reg32_t)(USB_BASE + 0x0c68) //  RW 32 0xffffffff
#define USB_DOEPTSIZ11     (reg32_t)(USB_BASE + 0x0c70) //  RW 32 0xffffffff
#define USB_DOEPDMA11      (reg32_t)(USB_BASE + 0x0c74) //  RW 32 0xffffffff

#define USB_DOEPCTL12      (reg32_t)(USB_BASE + 0x0c80) //  RW 32 0xffffffff
#define USB_DOEPINT12      (reg32_t)(USB_BASE + 0x0c88) //  RW 32 0xffffffff
#define USB_DOEPTSIZ12     (reg32_t)(USB_BASE + 0x0c90) //  RW 32 0xffffffff
#define USB_DOEPDMA12      (reg32_t)(USB_BASE + 0x0c94) //  RW 32 0xffffffff

#define USB_DOEPCTL13      (reg32_t)(USB_BASE + 0x0ca0) //  RW 32 0xffffffff
#define USB_DOEPINT13      (reg32_t)(USB_BASE + 0x0ca8) //  RW 32 0xffffffff
#define USB_DOEPTSIZ13     (reg32_t)(USB_BASE + 0x0cb0) //  RW 32 0xffffffff
#define USB_DOEPDMA13      (reg32_t)(USB_BASE + 0x0cb4) //  RW 32 0xffffffff

#define USB_DOEPCTL14      (reg32_t)(USB_BASE + 0x0cc0) //  RW 32 0xffffffff
#define USB_DOEPINT14      (reg32_t)(USB_BASE + 0x0cc8) //  RW 32 0xffffffff
#define USB_DOEPTSIZ14     (reg32_t)(USB_BASE + 0x0cd0) //  RW 32 0xffffffff
#define USB_DOEPDMA14      (reg32_t)(USB_BASE + 0x0cd4) //  RW 32 0xffffffff

#define USB_DOEPCTL15      (reg32_t)(USB_BASE + 0x0ce0) //  RW 32 0xffffffff
#define USB_DOEPINT15      (reg32_t)(USB_BASE + 0x0ce8) //  RW 32 0xffffffff
#define USB_DOEPTSIZ15     (reg32_t)(USB_BASE + 0x0cf0) //  RW 32 0xffffffff
#define USB_DOEPDMA15      (reg32_t)(USB_BASE + 0x0cf4) //  RW 32 0xffffffff

#define USB_PCGCR          (reg32_t)(USB_BASE + 0x0e00) //  RW 4 0x0000000f

#define USB_DFIFO0         (reg32_t)(USB_BASE + 0x1000) //  RW 32 0xffffffff
#define USB_DFIFO1         (reg32_t)(USB_BASE + 0x2000) //  RW 32 0xffffffff
#define USB_DFIFO2         (reg32_t)(USB_BASE + 0x3000) //  RW 32 0xffffffff
#define USB_DFIFO3         (reg32_t)(USB_BASE + 0x4000) //  RW 32 0xffffffff
#define USB_DFIFO4         (reg32_t)(USB_BASE + 0x5000) //  RW 32 0xffffffff
#define USB_DFIFO5         (reg32_t)(USB_BASE + 0x6000) //  RW 32 0xffffffff
#define USB_DFIFO6         (reg32_t)(USB_BASE + 0x7000) //  RW 32 0xffffffff
#define USB_DFIFO7         (reg32_t)(USB_BASE + 0x8000) //  RW 32 0xffffffff
#define USB_DFIFO8         (reg32_t)(USB_BASE + 0x9000) //  RW 32 0xffffffff
#define USB_DFIFO9         (reg32_t)(USB_BASE + 0xa000) //  RW 32 0xffffffff
#define USB_DFIFO10        (reg32_t)(USB_BASE + 0xb000) //  RW 32 0xffffffff
#define USB_DFIFO11        (reg32_t)(USB_BASE + 0xc000) //  RW 32 0xffffffff
#define USB_DFIFO12        (reg32_t)(USB_BASE + 0xd000) //  RW 32 0xffffffff
#define USB_DFIFO13        (reg32_t)(USB_BASE + 0xe000) //  RW 32 0xffffffff
#define USB_DFIFO14        (reg32_t)(USB_BASE + 0xf000) //  RW 32 0xffffffff
#define USB_DFIFO15        (reg32_t)(USB_BASE + 0x10000) //  RW 32 0xffffffff

#define USB_GOTGCTL_SES_REQ_SCS               0//  0 0x00000001 0xfffffffe 0x0
#define USB_GOTGCTL_SES_REQ                   1//  1 0x00000002 0xfffffffd 0x0
#define USB_GOTGCTL_RES0                      2//  7 NA NA NA
#define USB_GOTGCTL_HST_NEG_SCS               8//  8 0x00000100 0xfffffeff 0x0
#define USB_GOTGCTL_HNP_REQ                   9//  9 0x00000200 0xfffffdff 0x0
#define USB_GOTGCTL_HST_SET_HNP_EN           10//  10 0x00000400 0xfffffbff 0x0
#define USB_GOTGCTL_DEV_HNP_EN               11//  11 0x00000800 0xfffff7ff 0x0
#define USB_GOTGCTL_RES1                     12//  15 NA NA NA
#define USB_GOTGCTL_CON_ID_STS               16//  16 0x00010000 0xfffeffff 0x0
#define USB_GOTGCTL_DBNC_TIME                17//  17 0x00020000 0xfffdffff 0x0
#define USB_GOTGCTL_A_SES_VLD                18//  18 0x00040000 0xfffbffff 0x0
#define USB_GOTGCTL_B_SES_VLD                19//  19 0x00080000 0xfff7ffff 0x0

#define USB_GUSBCFG_TOUT_CAL              0//  2 0x00000007 0xfffffff8 0x0
#define USB_GUSBCFG_PHY_IF                3//  3 0x00000008 0xfffffff7 0x0
#define USB_GUSBCFG_ULPI_UTMI_SEL         4//  4 0x00000010 0xffffffef 0x0
#define USB_GUSBCFG_FS_INTF               5//  5 0x00000020 0xffffffdf 0x0
#define USB_GUSBCFG_PHY_SEL               6//  6 0x00000040 0xffffffbf 0x0
#define USB_GUSBCFG_DDR_SEL               7//  7 0x00000080 0xffffff7f 0x0
#define USB_GUSBCFG_SRP_CAP               8//  8 0x00000100 0xfffffeff 0x0
#define USB_GUSBCFG_HNP_CAP               9//  9 0x00000200 0xfffffdff 0x0
#define USB_GUSBCFG_USB_TRD_TIM          10//  13 0x00003c00 0xffffc3ff 0x0
#define USB_GUSBCFG_UNKNOWN              14//  14 NA NA NA
#define USB_GUSBCFG_PHY_LPWR_CLK_SEL     15//  15 0x00008000 0xffff7fff 0x0
#define USB_GUSBCFG_OTG_I2C_SEL          16//  16 0x00010000 0xfffeffff 0x0
#define USB_GUSBCFG_ULPI_FS_LS           17//  17 0x00020000 0xfffdffff 0x0
#define USB_GUSBCFG_ULPI_AUTO_RES        18//  18 0x00040000 0xfffbffff 0x0
#define USB_GUSBCFG_ULPI_CLK_SUS_M       19//  19 0x00080000 0xfff7ffff 0x0
#define USB_GUSBCFG_ULPI_EXT_VBUS_DRV    20//  20 0x00100000 0xffefffff 0x0
#define USB_GUSBCFG_ULPI_EXT_VBUS_IND    21//  21 0x00200000 0xffdfffff 0x0
#define USB_GUSBCFG_TERM_SEL_DL_PULSE    22//  22 0x00400000 0xffbfffff 0x0
#define USB_GUSBCFG_IND_COMP             23//  23 0x00800000 0xff7fffff 0x0
#define USB_GUSBCFG_IND_PASS_THRU        24//  24 0x01000000 0xfeffffff 0x0
#define USB_GUSBCFG_ULPI_IF_PROT_DIS     25//  25 0x02000000 0xfdffffff 0x0
#define USB_GUSBCFG_UNKNOWN_2            26//  28 NA NA NA
#define USB_GUSBCFG_FORCE_HST_MODE       29//  29 0x20000000 0xdfffffff 0x0
#define USB_GUSBCFG_FORCE_DEV_MODE       30//  30 0x40000000 0xbfffffff 0x0
#define USB_GUSBCFG_CORRUPT_TX           31//  31 0x80000000 0x7fffffff 0x0

#define USB_GAHBCFG_GLBL_INTR_MSK             0//  0 0x00000001 0xfffffffe 0x0
#define USB_GAHBCFG_H_BST_LEN                 1//  4 0x0000001e 0xffffffe1 0x0
#define USB_GAHBCFG_DMA_EN                    5//  5 0x00000020 0xffffffdf 0x0
#define USB_GAHBCFG_UNKNOWN                   6//  6 NA NA NA
#define USB_GAHBCFG_NP_TXF_EMP_LVL            7//  7 0x00000080 0xffffff7f 0x0
#define USB_GAHBCFG_P_TXF_EMP_LVL             8//  8 0x00000100 0xfffffeff 0x0
#define USB_GAHBCFG_DMA_REM_MODE             23//  8 0x00000100 0xfffffeff 0x0

#define USB_GNPTXFSIZ_NP_TXF_ST_ADDR          0//  15 0x0000ffff 0xffff0000 0x0
#define USB_GNPTXFSIZ_IN_EP_TXF0_ST_ADDR      0//  15 0x0000ffff 0xffff0000 0x0
#define USB_GNPTXFSIZ_IN_EP_TXF0_DEP         16//  31 0xffff0000 0x0000ffff 0x0
#define USB_GNPTXFSIZ_NP_TXF_DEP             16//  31 0xffff0000 0x0000ffff 0x0

/* 0: Connection status: 1 - has connection , 0 - no connection 
 * 1: Connection detected: 1 - connection detected, 0 - not, write 1 to clear
 * 2: Enabled: 1 - enabled, 0 - not enabled                , write 1 to clear
 * 3: Enabled changed: enabled status changed,             , write 1 to clear
 * 4: Overcurrent status: 1 overcurrent, 0 - no overcurr
 * 5: Overcurrent status change:                           , write 1 to clear
 * 6: Suspended  1 - suspend / 0 - not
 * 8: Reset      1 - assert reset, 0 - deassert reset
 * 10: Line status ??
 * 12: Power :   1 - enable / 0 -disable
 * 17: Speed ???
 */

#define USB_HPRT_CONN_STS                     0//  0 0x00000001 0xfffffffe 0x0
#define USB_HPRT_CONN_DET                     1//  1 0x00000002 0xfffffffd 0x0
#define USB_HPRT_ENA                          2//  2 0x00000004 0xfffffffb 0x0
#define USB_HPRT_EN_CHNG                      3//  3 0x00000008 0xfffffff7 0x0
#define USB_HPRT_OVRCURR_ACT                  4//  4 0x00000010 0xffffffef 0x0
#define USB_HPRT_OVRCURR_CHNG                 5//  5 0x00000020 0xffffffdf 0x0
#define USB_HPRT_RES                          6//  6 0x00000040 0xffffffbf 0x0
#define USB_HPRT_SUSP                         7//  7 0x00000080 0xffffff7f 0x0
#define USB_HPRT_RST                          8//  8 0x00000100 0xfffffeff 0x0
#define USB_HPRT_RES0                         9//  9   NA    NA    NA 
#define USB_HPRT_LN_STS                      10//  11 0x00000c00 0xfffff3ff 0x0
#define USB_HPRT_PWR                         12//  12 0x00001000 0xffffefff 0x0
#define USB_HPRT_TST_CTL                     13//  16 0x0001e000 0xfffe1fff 0x0
#define USB_HPRT_SPD                         17//  18 0x00060000 0xfff9ffff 0x0

#define USB_HPRT_WRITE_CLEAR_BITS \
   ((1<<USB_HPRT_OVRCURR_CHNG)|\
    (1<<USB_HPRT_EN_CHNG     )|\
    (1<<USB_HPRT_ENA         )|\
    (1<<USB_HPRT_CONN_DET    ))

#define USB_HPRT_WRITE_MASK ~(uint32_t)USB_HPRT_WRITE_CLEAR_BITS

#define USB_HCTSIZ0_PID_DATA0 0
#define USB_HCTSIZ0_PID_DATA1 2
#define USB_HCTSIZ0_PID_DATA2 1
#define USB_HCTSIZ0_PID_SETUP 3

#define USB_HOST_CHAR_MAX_PACK_SZ_BF  0, 11
#define USB_HOST_CHAR_EP_BF           11, 4
#define USB_HOST_CHAR_EP_DIR_BF       15, 1
#define USB_HOST_CHAR_IS_LOW_BF       17, 1
#define USB_HOST_CHAR_EP_TYPE_BF      18, 2
#define USB_HOST_CHAR_PACK_PER_FRM_BF 20, 2
#define USB_HOST_CHAR_DEV_ADDR_BF     22, 7
#define USB_HOST_CHAR_ODD_FRAME_BF    29, 1
#define USB_HOST_CHAR_CHAN_DISABLE_BF 30, 1
#define USB_HOST_CHAR_CHAN_ENABLE_BF  31, 1

#define USB_HOST_CHAR_GET_PORT_ADDR_BF      0, 7
#define USB_HOST_CHAR_GET_HUB_ADDR_BF       7, 7
#define USB_HOST_CHAR_GET_TRANS_POS_BF      14, 2
#define USB_HOST_CHAR_GET_COMPLETE_SPLIT_BF 16, 1
#define USB_HOST_CHAR_GET_SPLT_ENABLE_BF    31, 1
