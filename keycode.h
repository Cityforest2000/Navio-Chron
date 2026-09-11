#define KC_BASE 0     // General Keycode
#define KC_NO -1
#define KC_MAX 255


#define KP_BASE 0x0100      // Key Pressed
#define KP_SHIFT 0x0100
#define KP_CTRL 0x0200
#define KP_ALT 0x0400
#define KP_FN 0x0800
#define KP_WIN 0x1000
#define KP_MAX 0xFFFF

#define KEY_BACKSPACE 0xB2
#define KEY_LEFT_CTRL 0x80
#define KEY_LEFT_SHIFT 0x81
#define KEY_LEFT_ALT 0x82

#define KM_BASE 0x10000     // Keyboard MACRO
#define KM_MACRO0 0x10000
#define KM_MACRO1 0x10001
#define KM_MACRO2 0x10002
#define KM_MACRO3 0x10003
#define KM_MACRO4 0x10004
#define KM_MACRO5 0x10005
#define KM_MACRO6 0x10006
#define KM_MACRO7 0x10007
#define KM_MACRO8 0x10008
#define KM_MACRO9 0x10009
#define KM_MACRO10 0x10010

#define L_BASE 0x300
#define LAYER0  0x300
#define LAYER1  0x301
#define LAYER2  0x302
#define LAYER3  0x303
#define LAYER4  0x304
#define LAYER5  0x305
#define LAYER6  0x306
#define L_BASE1 0x310
#define L_TOGGLE  0x311
#define L_BASE2 0x320
#define L_TRANS0  0x500 // Trans to Layer0

#define KMF_BASE 0x1000000     // Multi Function
#define KMF_FN1  0x1311301
#define KMF_FN2  0x1000000 | (' '<<12) | 0x302
#define KMF_FN3  0x1000000 | (' '<<12) | 0x303
#define KMF_FN5  0x1311305
#define KMF_BASE1 0x2000000
#define KMF(a,b) 0x1000000 | (a << 12) | b

#define MOUSEMOVE(x,y,wheel,pan) ((x & 0xFF) << 24) | ((y & 0xFF) << 16) | ((wheel & 0xFF) << 8) | (pan & 0xFF)


