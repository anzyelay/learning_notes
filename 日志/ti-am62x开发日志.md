# TI AM62LP

## GPIO

### AM62的GPIO组成

- MAIN DOMAIN
  - GPIO0: 0~91 (eg: GPIO0_0, GPIO0_9)
  - GPIO1: 0~51 (eg: GPIO1_0, GPIO1_51)
- MCU DOMAIN
  - MCU_GPIO0: 0~23

### userspace write/read

- tools

    tool | function
    -|-
    gpiodetect | List GPIO chips, print their labels and number of GPIO lines.
    gpioset | gpioset -c chip line=value
    gpioget | gpioget -c chip line
    gpioinfo | Lines are specified by name, or optionally by offset if the chip option is provided.

- 通过`gpiodetect`查看每个gpiochip的lines数:

    ```sh
    root@fvt-5g-tbox:~# gpiodetect
    gpiochip0 [tps65219-gpio] (3 lines)
    gpiochip1 [1-0023] (24 lines)
    gpiochip2 [600000.gpio] (92 lines)
    gpiochip3 [601000.gpio] (52 lines)
    gpiochip4 [1-0022] (24 lines)
    ```

    根据[AM62的GPIO组成](#am62的gpio组成)可知，GPIO0_X组有92个GPIO，所以是gpiochip2, 同理GPIO1_X为gpiochip3

- 配置输出：`devmem2 gpio_address 0x00010007`

- 拉高: `gpioset -c chip line=1`
