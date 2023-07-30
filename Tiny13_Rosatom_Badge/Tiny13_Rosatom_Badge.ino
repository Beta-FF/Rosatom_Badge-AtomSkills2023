#include <util/delay.h>

#define STRIP_PIN   8     // пин ленты
#define NUMLEDS     8      // кол-во светодиодов
#define MOSFET_PIN  4//PB4
#define BTN_PIN     0//PB0

#define PRESS       0
#define NOT_PRESS   1

#define MAX_BRIGHT  10

#define STEP1   16
#define STEP2   2
#define TM      10
// 20 ~ 7mA max
// 10 ~ 3mA max
// 5  ~ 1.5mA max


uint8_t led_color[3 * (NUMLEDS - 1)];
uint8_t btn_old = NOT_PRESS;
uint8_t btn_hold = 0;
uint8_t mode = 0;
bool inc = true;
uint8_t dynamic_color = MAX_BRIGHT;
uint8_t dot_cnt = 0;
uint8_t timer_cnt = 0;
uint8_t color = 0;

void Wheel(byte WheelPos,uint8_t n);
void ws2812b_send_color_no_reset(const uint8_t pin_value, const uint8_t red, const uint8_t green, const uint8_t blue);

static void ws2812b_send_color(const uint8_t pin_value, const uint8_t red, const uint8_t green, const uint8_t blue) {
    DDRB |= pin_value;
    PORTB &= ~pin_value;
    _delay_us(50);
    ws2812b_send_color_no_reset(pin_value, red, green, blue);
}

void set_dot_ws2812(uint8_t position, uint8_t col_r, uint8_t col_g, uint8_t col_b) {
  if(position < NUMLEDS) {
        led_color[position * 3] = col_r;
        led_color[(position * 3) + 1] = col_g;
        led_color[(position * 3) + 2] = col_b;
  }
}

void set_full_ws2812(uint8_t col_r, uint8_t col_g, uint8_t col_b) {
    for(uint8_t i=0; i < NUMLEDS; i++) {
        led_color[i * 3] = col_r;
        led_color[(i * 3) + 1] = col_g;
        led_color[(i * 3) + 2] = col_b;
    }
}

void sync_ws2812() {
    ws2812b_send_color(STRIP_PIN, led_color[0], led_color[1], led_color[2]);
    for(uint8_t i=1; i<NUMLEDS; i++) {
        ws2812b_send_color_no_reset(STRIP_PIN, led_color[i * 3], led_color[(i * 3) + 1], led_color[(i * 3) + 2]);
    }
}

void mode_handler() {
    switch (mode) {
        case 0:
            set_full_ws2812(((dynamic_color >> 1) - (MAX_BRIGHT / 2)) * 2, (dynamic_color >> 1), MAX_BRIGHT);
            if(inc) dynamic_color++;
            else dynamic_color--;
            if(dynamic_color >= MAX_BRIGHT * 2) inc = false;
            if(dynamic_color <= MAX_BRIGHT)  inc = true;
            break;
        case 1:
            set_full_ws2812(0, MAX_BRIGHT * 0.5, MAX_BRIGHT);
            set_dot_ws2812(dot_cnt, MAX_BRIGHT * 0.7, MAX_BRIGHT * 0.7, MAX_BRIGHT * 0.7);
            set_dot_ws2812(dot_cnt + 1, MAX_BRIGHT * 0.7, MAX_BRIGHT * 0.7, MAX_BRIGHT * 0.7);
            if(timer_cnt == 0) {
                if(inc) dot_cnt++;
                else {
                    if(dot_cnt > 0) dot_cnt--;
                }
                if(dot_cnt >= NUMLEDS - 2) {
                    inc = false;
                    timer_cnt = 5;
                }
                if(dot_cnt == 0)  {
                    inc = true;
                    timer_cnt = 5;
                }
            }
            else timer_cnt--;
            break;
        case 2:
            set_full_ws2812(MAX_BRIGHT * 0.7, MAX_BRIGHT * 0.7, MAX_BRIGHT * 0.7);
            break;
        // case 3:
        //     set_full_ws2812(0, MAX_BRIGHT * 0.5, MAX_BRIGHT);
        //     break;
        case 3:
            for(int i=0; i< NUMLEDS; i++ ){
                Wheel(color + i*STEP1, i );    
                color += STEP2;
                delay(TM);
            }
            break;
    }
}

void button_press() {

}

void button_release() {
    if(++mode > 3) mode = 0;
}

void button_hold() {
    digitalWrite(MOSFET_PIN, HIGH);
}

void button_handler() {
    if(digitalRead(BTN_PIN) == PRESS) {
        if(btn_old == NOT_PRESS) button_press();
        btn_hold++;
        if(btn_hold >= 10) button_hold();
        btn_old = PRESS;
    } else {
        if(btn_old == PRESS) {
            if(btn_hold >= 10) ;//button_hold();
            else button_release();
            btn_hold = 0;
        }
        btn_old = NOT_PRESS;
    }
}

void setup() {
    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, LOW);
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(3, OUTPUT);
}

void loop() {
    button_handler();
    mode_handler();
    sync_ws2812();
    delay(100);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void Wheel(byte WheelPos,uint8_t n) {
  if(WheelPos < 85) {
     led_color[n * 3] = MAX_BRIGHT * (WheelPos * 3) / 255;
     led_color[n * 3 + 1] = MAX_BRIGHT * (255 - WheelPos * 3) / 255;
     led_color[n * 3 + 2] = 0;
  } 
  else if(WheelPos < 170) {
     WheelPos -= 85;
     led_color[n * 3] = MAX_BRIGHT * (255 - WheelPos * 3) / 255;
     led_color[n * 3 + 1] = 0;
     led_color[n * 3 + 2] = MAX_BRIGHT * (WheelPos * 3) / 255;
  } 
  else {
     WheelPos -= 170;
     led_color[n * 3] = 0;
     led_color[n * 3 + 1] = MAX_BRIGHT * (WheelPos * 3) / 255;
     led_color[n * 3 + 2] = MAX_BRIGHT * (255 - WheelPos * 3) / 255;
  }
}

void ws2812b_send_color_no_reset(const uint8_t pin_value, const uint8_t red, const uint8_t green, const uint8_t blue) {
    DDRB |= pin_value;
    const uint8_t portb = PORTB; // PORTB is volatile, so preload value
    const uint8_t lo = portb & ~pin_value;
    const uint8_t hi = portb | pin_value;
    __asm__  volatile (   
                // Each 12 cycles go high on cycle 0 and go low on cycle:
                //   - 8 if a one bit is transmitted
                //   - 4 if a zero bit is transmitted
                //
                // In the spare time we have left, we read out the next bit from the current color
                // byte. We keep a mask bit in r17, do a bitwise and operation, and branch if (not)
                // zero. After 8 shifts the carry flag will be set and we will move on to the next
                // color bit.
                //
                // I have figured that it should be possible to decode the bits from the RGB values
                // on the fly. However, I think that we do not have enough cycles to read from the
                // different color in a non-unrolled loop, but to be completely honest I did not
                // take a lot of effort to find a good argument on why this should be impossible.
                //
                ".green:                        \n\t"
                "mov r16,%[green]               \n\t"
                "ldi r17,0x80                   \n\t"
                "and r16,r17                    \n\t"
                "brne .green_transmit_one_0     \n\t"
                "rjmp .green_transmit_zero_0    \n\t"
                
                // Green
                ".green_transmit_one_1:         \n\t"
                "nop                            \n\t" // cycle -1
                ".green_transmit_one_0:         \n\t"
                "out 0x18,%[hi]                 \n\t" // cycle 0
                "nop                            \n\t" // cycle 1
                "lsr r17                        \n\t" // cycle 2
                "brcs .green_transmit_one_done  \n\t" // cycle 3
                "rjmp .+0                       \n\t" // cycle 4
                "mov r16,%[green]               \n\t" // cycle 6
                "and r16,r17                    \n\t" // cycle 7
                "out 0x18,%[lo]                 \n\t" // cycle 8
                "brne .green_transmit_one_1     \n\t" // cycle 9
                "rjmp .green_transmit_zero_0    \n\t" // cycle 10
                
                ".green_transmit_one_done:      \n\t"
                "ldi r17,0x80                   \n\t" // cycle 5
                "mov r16,%[red]                 \n\t" // cycle 6
                "and r16,r17                    \n\t" // cycle 7
                "out 0x18,%[lo]                 \n\t" // cycle 8
                "brne .red_transmit_one_1       \n\t" // cycle 9
                "rjmp .red_transmit_zero_0      \n\t" // cycle 10
                
                ".green_transmit_zero_0:        \n\t"
                "out 0x18,%[hi]                 \n\t" // cycle 0
                "lsr r17                        \n\t" // cycle 1
                "brcs .green_transmit_zero_done \n\t" // cycle 2
                "nop                            \n\t" // cycle 3
                "out 0x18,%[lo]                 \n\t" // cycle 4
                "rjmp .+0                       \n\t" // cycle 5
                "mov r16,%[green]               \n\t" // cycle 7
                "and r16,r17                    \n\t" // cycle 8
                "brne .green_transmit_one_1     \n\t" // cycle 9
                "rjmp .green_transmit_zero_0    \n\t" // cycle 10
                
                ".green_transmit_zero_done:     \n\t"
                "out 0x18,%[lo]                 \n\t" // cycle 4
                "nop                            \n\t" // cycle 5
                "ldi r17,0x80                   \n\t" // cycle 6
                "mov r16,%[red]                 \n\t" // cycle 7
                "and r16,r17                    \n\t" // cycle 8
                "brne .red_transmit_one_1       \n\t" // cycle 9
                "rjmp .red_transmit_zero_0      \n\t" // cycle 10
                
                // Red
                ".red_transmit_one_1:           \n\t"
                "nop                            \n\t" // cycle -1
                ".red_transmit_one_0:           \n\t"
                "out 0x18,%[hi]                 \n\t" // cycle 0
                "nop                            \n\t" // cycle 1
                "lsr r17                        \n\t" // cycle 2
                "brcs .red_transmit_one_done    \n\t" // cycle 3
                "rjmp .+0                       \n\t" // cycle 4
                "mov r16,%[red]                 \n\t" // cycle 6
                "and r16,r17                    \n\t" // cycle 7
                "out 0x18,%[lo]                 \n\t" // cycle 8
                "brne .red_transmit_one_1       \n\t" // cycle 9
                "rjmp .red_transmit_zero_0      \n\t" // cycle 10
                
                ".red_transmit_one_done:        \n\t"
                "ldi r17,0x80                   \n\t" // cycle 5
                "mov r16,%[blue]                \n\t" // cycle 6
                "and r16,r17                    \n\t" // cycle 7
                "out 0x18,%[lo]                 \n\t" // cycle 8
                "brne .blue_transmit_one_1      \n\t" // cycle 9
                "rjmp .blue_transmit_zero_0     \n\t" // cycle 10
                
                ".red_transmit_zero_0:          \n\t"
                "out 0x18,%[hi]                 \n\t" // cycle 0
                "lsr r17                        \n\t" // cycle 1
                "brcs .red_transmit_zero_done   \n\t" // cycle 2
                "nop                            \n\t" // cycle 3
                "out 0x18,%[lo]                 \n\t" // cycle 4
                "rjmp .+0                       \n\t" // cycle 5
                "mov r16,%[red]                 \n\t" // cycle 7
                "and r16,r17                    \n\t" // cycle 8
                "brne .red_transmit_one_1       \n\t" // cycle 9
                "rjmp .red_transmit_zero_0      \n\t" // cycle 10
                
                ".red_transmit_zero_done:       \n\t"
                "out 0x18,%[lo]                 \n\t" // cycle 4
                "nop                            \n\t" // cycle 5
                "ldi r17,0x80                   \n\t" // cycle 6
                "mov r16,%[blue]                \n\t" // cycle 7
                "and r16,r17                    \n\t" // cycle 8
                "brne .blue_transmit_one_1      \n\t" // cycle 9
                "rjmp .blue_transmit_zero_0     \n\t" // cycle 10
                
                // Blue
                ".blue_transmit_one_1:          \n\t"
                "nop                            \n\t" // cycle -1
                ".blue_transmit_one_0:          \n\t"
                "out 0x18,%[hi]                 \n\t" // cycle 0
                "rjmp .+0                       \n\t" // cycle 1
                "lsr r17                        \n\t" // cycle 3
                "brcs .blue_transmit_one_done   \n\t" // cycle 4
                "nop                            \n\t" // cycle 5
                "mov r16,%[blue]                \n\t" // cycle 6
                "and r16,r17                    \n\t" // cycle 7
                "out 0x18,%[lo]                 \n\t" // cycle 8
                "brne .blue_transmit_one_1      \n\t" // cycle 9
                "rjmp .blue_transmit_zero_0     \n\t" // cycle 10
                
                ".blue_transmit_one_done:       \n\t"
                "rjmp .blue_transmit_zero_done  \n\t" // cycle 6
                
                ".blue_transmit_zero_0:         \n\t"
                "out 0x18,%[hi]                 \n\t" // cycle 0
                "lsr r17                        \n\t" // cycle 1
                "brcs .blue_transmit_zero_done  \n\t" // cycle 2
                "nop                            \n\t" // cycle 3
                "out 0x18,%[lo]                 \n\t" // cycle 4
                "rjmp .+0                       \n\t" // cycle 5
                "mov r16,%[blue]                \n\t" // cycle 7
                "and r16,r17                    \n\t" // cycle 8
                "brne .blue_transmit_one_1      \n\t" // cycle 9
                "rjmp .blue_transmit_zero_0     \n\t" // cycle 10
                
                ".blue_transmit_zero_done:      \n\t"
                "out 0x18,%[lo]                 \n\t" // cycle 4 / cycle 8
                ".end:                          \n\t"
            
            : // No outputs
            : [lo] "r" (lo), [hi] "r" (hi), [green] "r" (green), [red] "r" (red), [blue] "r" (blue)
            : "r16", "r17"
    );
}
