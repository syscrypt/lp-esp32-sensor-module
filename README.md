# Low Energy Environmental Sensor

Test project for a low power modular environmental sensor architecture that runs on an esp32 using the espressif idf. The main purpose is to periodically collect and store environmental data via a bme280 sensor. The data then can be sent via multiple preregistered protocols like ble, wifi or zigbee. The modular protocol interface allows for an easy integration of new protocols.

In this document I describe the components I used building this project but it's not limited strictly to them and other parts can be exchanged or added of course.

# Hardware Setup

![circuit-raw2](https://user-images.githubusercontent.com/103246733/201546922-76151010-a4b2-45a9-a773-e0bcb8ca8baf.png)

## XBee S2C 802.15.4 HF-Module

Since the ESP32 isn’t capable of the ZigBee protocol, an external controller is used for
transmitting and receiving ZigBee messages. The XBee S2C HF-Module from Digi Interna-
tional® provides an interface for the ZigBee communication protocol. It can operate as an
End Device, Router or Coordinator within the ZigBee network, while reaching distances up
to 60 meter or 90 meter for the XBee-PRO module. The supply voltage is between 2.1 to
3.6 volts, with a typical transmit current (VCC = 3.3 V) of 33 mA (5 dBm, Normal mode)
or 45 mA (8 dBm, Boost mode) and a typical idle / receive current (VCC = 3.3 V) of 28
mA (Normal mode) or 31 mA (Boost mode). By exploiting the modules SLEEP_RQ pin,
it can be put into sleep mode in which the controller consumes less than 1 μA current at an
operation temperature of 25°C.

## Push Button

In order to transition the sensor into a maintenance/configuration mode, it needs to be
externally triggerable. This could be achieved by using any low range sensors like ultrasonic
Hardware Setup 16
or infrared but for the purpose of measurement, a simple push button will be used to save
the effort of manually disabling the sensor. An enabled sensor could lead to an accidental
trigger of the wakeup event, which could cause interference in the measurement process. The
push button is connected to a 1 kΩ pull-down resistor.

## ESP32-WROOM-32

The ESP32 defines a series of low power, standalone 32-bit microcontrollers, designed and
engineered for mobile devices, wearable electronics and IoT applications. It provides a wide
range of low power settings as well as integrates Wi-Fi and Bluetooth functionallity, which
can be controlled via it’s set of interfaces (Serial Peripheral Interface (SPI) / Secure Digitial
Input Output (SDIO), Inter Integrated Circuit (I2C) / Universal Asynchronous Receiver
Transmitter (UART)). The operating system is freeRTOS with LwIP, which supports mul-
titasking and hardware accelerated encryption. The ESP32 is well suited for a wide range
of mobile applications, it comes with either one or two cpu cores with an adjustable clock
frequency, ranging from 80 MHz to 240 MHz. For this work, the ESP32-WROOM-32/ESP32-
D0WDQ6 was chosen, as it offers a great flexibility and comes with a low-power co-processor
Hardware Setup 15
for power saving purposes. The power supply voltage range is between 3.0 to 3.6 volts, the
controller operates from -40°C up to +85°C, while it draws an average current of 80 mA.
The chip has an internal flash storage of 4 MB size to store arbitrary data, while providing
520 KB of on-chip SRAM and 16 KB of Static Random Access Memory (SRAM) in Real
Time Clock (RTC) to use in association with the deep sleep mode.
These attributes together with a sleep currency of less than 5μ ampere led to the decision
of using this controller as base for the sensor module built up throughout this thesis. Table
6.1 provides an overview of the controllers power consumption in different modes, it also
presents a reference of the RF modules power consumption.

The BME280 developed by Bosch is a low power I2C sensor, which has the ability to measure
relative humidity, air pressure as well as temperature. The controller operates with a voltage
supply between 1.71 to 3.6 volts, with an average current consumption (1 Hz data refresh
rate) of 1.8 μA for relative humidity/temperature, 2.8 μA for pressure/temperature and 3.6
μA for all three measurements. Further the operation temperature range is between -40°C to
85°C and the operation pressure between 300 to 1100 hPa. The current consumption in sleep
mode is pretty low with an average of 0.1 μA. Due to the low overall power consumption,
the BME280 can be powered directly from the GPIO of the esp32.

## Battery

The battery, even if not necessary for measuring, can have a drastical impact on the power
efficiency of the sensor. A high voltage needs to be regulated by either an external regulator
or by the internal one of the ESP32, if the voltage is below 12 V. Further a non quiet
regulator can lead to interference of the RF modules. The ESP32 alone has a maximum
current consumption of 400 mA, which could be exceeded by external modules that run
in parallel, so the battery should be able to handle these peaks. For the module to build
throughout this work, a LiFePo4 (IFR 18500J) battery is used, which provides a constant
power supply of 3.2 V with a capacity of 1100 mAh to 1200 mAh, while allowing a max.
discharge current of 5.5 A.

# Configuration

In order to control the modules behaviour, a data object of type struct is defined and
displayed in the following listing. These parameters
have a direct impact on the total energy consumption of the sensor, which will be examined
in the further course of this work.

```c
typedef struct {
    uint32_t deep_sleep_ms;
    uint32_t wakeup_counter_threshold;
    uint8_t send_last_x_values;
    uint8_t activated_protocol;
    uint8_t mode;
} config_t;
```

## Deep Sleep Interval

Represents the time interval in milliseconds, which determines how long the sensor remains
in the deep sleep mode before receiving the timer wakeup interrupt. This property is of
type `uint32_t`, which allows a maximum sleep duration of approximately 1190 hours and is
referenced by the `deep_sleep_ms` field of the `config_t` struct.

## Wakeup Counter Threshold

The wakeup counter is incremented each time the module transitions from the deep sleep
mode and the wakeup source is the timer. If the counter reaches the threshold defined by
this property, the stored measurement data gets transmitted. This value is referenced by
the `deep_sleep_ms` field of the `config_t` struct.

## Send Value Batch

Determines the size of the measurement data entry batch to be send, after reaching the
threshold. The batch contains only the latest measurement values. This value is referenced
by the `send_last_x_values` field of the `config_t` struct.

## Activated Protocol

Specifies the communication protocol that is used for the data transmission. Each user regis-
tered protocol is identified by an unique identification byte, assigned during the initialization
process of the protocol. Only one communication protocol can be active at a time. Even
though it is possible to change the protocol in the configurations, the inactive protocols
are not initialized and no other resources besides storage space are required. This value is
referenced by the `activated_protocol` field of the `config_t` struct.
Mode
The mode property controlls the behaviour of the module by defining a set of flags which
enable or disable certain procedures. The value combinations are displayed in the following table,
whereby the `Value` column refers to the flags concrete value in hexadecimal, the `Mode` column
captions the mode for a human readable representation, the `Valid` column indicates if the
value refers to an executable configuration and the `Description` column provides a brief
description of the resulting behaviour. This value is referenced by the mode field of the
`config_t` struct.

| Value | Mode                  | Valid | Description                                                                                         |
| ----- | --------------------- | ----- | --------------------------------------------------------------------------------------------------- |
| 0x00  | Empty                 | yes   | The sensor collects data while remaining in an active state, the sleep interval is used for a delay |
| 0x01  | Deep Sleep (DS)       | yes   | The sensor transitions into the deep sleep mode after data collection /transmission                 |
| 0x02  | Maintenance Mode (MM) | yes   | The sensor enters the maintenance mode after the next boot                                          |
| 0x04  | Wakeup Threshold (WT) | no    | Enables the wakeup threshold                                                                        |
| 0x05  | WT + DS               | yes   | Enables the deep sleep together with the wakeup threshold                                           |
| 0x06  | WT + MM               | no    | Enables the wakeup threshold and transitions into the maintenance mode after next boot              |
| 0x07  | MM + DS + WT          | yes   | Enters the maintenance mode after next boot while enabling                                          |

# Protocol

In order to provide an overview of the sensors behaviour and the interaction of its components,
the functionality of the module is divided into four separate states, which are
implemented individually. These states consists of four modes in which the sensor can operate.
The transitions between these modes are triggered by concrete operations, which follow
strict prior decisions.

## Data Collection Mode

In this mode, the module requests the measurement of the environmental data, that was
preconfigured in the settings and stores it into the internal flash memory of the controller,
after receiving the values from the sensor. After the measured data is successfully written,
a corresponding event gets emitted. The sensor then enters the deep sleep mode, if the
corresponding flag in the modules settings was activated.

## Deep Sleep Mode

In this mode, only the ultra low power co processor of the esp32 microcontroller is active.
Necessary areas e.g. for managing parts of the RAM of the ULP are active as well as the RTC
controller and RTC peripherals. This allows the sensor to maintain a certain set of variables
or to react to incoming signals. The module can exit the deep sleep mode by receiving a
preconfigured wakeup source. In this case the module boots up again. Two different wakeup
sources are defined by default: The timer wakeup source, that lets the module exit the
deep sleep state after a preconfigured time interval and the external wakeup source, which
is utilized to transition the sensor into maintenance mode. If an undefined wakeup source
is detected, e.g. after an unexpected disconnection from the power source, certain recovery
routines are called to reinitialize the undefined states in the RTC memory.

## Transmission Mode

In this mode, the collected data is transfered to another device via a selected communication
protocol for further processing. Depending on the settings, either a set of the last measured
values with a preconfigured size is transmitted or all measured values at once. The sensor
then enters the deep sleep mode, if the corresponding flag in the modules settings was
activated.

## Maintenance Mode

The purpose of the Maintenance Mode is to give the user the ability to directly communi-
cate with the sensor module in order to change configuration properties or trigger specific
procedures manually. After the sensor boots up, it first tries to queries the configuration
parameters from the flash, if they can’t be found, the module transitions into this mode
until a valid initial configuration is created. The Maintenance Mode covers a set of manually
triggerable commands, which are listed below.

# Mode Transitions

For a better visualization of the modes and their transitions, the following
nondeterministic Moore automata is defined as 7-tuple $A = (Q, &Sigma;, &Omega;,
&Delta;, &lambda;, q_0, &Gamma;)$, whereby $Q$ defines the set of states,
$&Sigma;$ is the input alphabet, $&Omega;$ defines the output alphabet,
$&Delta;$ represents the transition function, $&lambda;$ is the output function, $q_0$ is the start state
and $&Gamma;$ contains the set of accepted states.

$Q = \{MM, DCM, DSM, TM\}$  
$q_0 = MM$  
$&Sigma; = \{&varepsilon;, w_t, w_e, c_r, c_t, c_c, e_s \}$  
$&Omega; = \{md\}$  
$&Gamma; = \{MM\}$

For the benefit of the graphs clarity, acronyms are used for the
definition of the automat which are specified by the explenation of the graph.

The transition function $&Delta;$ along with the output function $&lambda;$ are
visualized using the following graph, where each $q_i \in &Gamma;$ is indicated
by a double circle and $q_0$ is represented by an arrow without prior state.

![state-machine2](https://user-images.githubusercontent.com/103246733/201546943-c1081e70-4a03-42dc-95d9-e19c30e2713a.png)

After powering up the sensor module, it transitions into the $MM$ state, which
represents the Maintenance Mode. This state is the only accepted state by the
sensor, as a further transition only happens, if the user interacts. If the restart
command $c_r$ is triggered, the module verifies the existence of a valid
configuration. If no configuration was found, the module reboots again into the
Maintenance Mode until the user writes an initial configuration, otherwise it
transitions into the $DCM$ state, which represents the Data Collection Mode.

The $DCM$ transitions either into the Deep Sleep Mode $DSM$ by
emitting the sleep event $e_s$ or into the Transmission Mode $TM$ if the
transmission threshold is reached. The $TM$ state emits similar to the $DCM$
state the sleep event $e_s$ after it successfully transmitted the data, which
also causes a transition into the $DSM$ state.
The $DSM$ state transitions either into the $DCM$ state if
it gets waked up by the timer wakeup $w_t$ or into the $MM$ state, if the wakeup
cause is an external wakeup $w_e$.
If the current state is the Transmition Mode, the module outputs a configured
set of measured data $md \in &Omega;$ via the selected communication protocol.
Transitions into the $DCM$ as well as the $TM$ state can both be triggered
manually from the $MM$ state by either releasing the transfer command $c_t$ to
transition into the Transmission Mode or by releasing the collect command $c_c$ to
transition into the Data Collection Mode. Both states transitions back into the
$MM$ state if that state caused the transition.
