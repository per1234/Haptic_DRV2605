/*  Haptic DRV2605 - Haptic Driver for Dialog DRV2605
    Copyright (C) 2018,2019 PatternAgents, LLC

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Wire.h>
#include "Haptic_DRV2605_registers.h"

//! ----------------------------------------------------------
//! Actuator Meta-Data : actuator and library default configuration
//! "hard" actuator defines
#define ACTUATOR_HAPTIC_DEV			ERM		         //! Actuator Type
#define ACTUATOR_OP_MODE			REGISTER_MODE
#define ACTUATOR_BEMF_SENS_EN		1
#define ACTUATOR_FREQ_TRACK_EN		1
#define ACTUATOR_ACC_EN				1
#define ACTUATOR_RAPID_STOP_EN		1
#define ACTUATOR_AMP_PID_EN			0
#define ACTUATOR_NOM_MV				300				//! Voltage setting / Unit: mili Volt 
#define ACTUATOR_OVERDRIVE_mV		3300			//! Voltage setting / Unit: mili Volt 
#define ACTUATOR_ABS_MAX_mV			5000
#define ACTUATOR_RESONANT_FREQ_Hz	180
#define ACTUATOR_IMAX_mA			137
#define ACTUATOR_IMPD_mOhm			(10500)			//! in milliOhm units 
#define ACTUATOR_RISE_TIME_mS       50              //! rise time in milliseconds
#define ACTUATOR_BRAKE_TIME_mS      50              //! braking time in milliseconds
#define ACTUATOR_GPI_0_MOD			DRV2605_SINGLE_PTN
#define ACTUATOR_GPI_0_POL			DRV2605_BOTH_EDGE

//! "soft" actuator defines
#define ACTUATOR_OVERIDE_VAL		0x59
#define ACTUATOR_SEQ_ID				7
#define ACTUATOR_SEQ_LOOP			3
#define ACTUATOR_SEQ_ID_MAX			15				//! SEQ_ID should not be larger than 15 (0 <= X <= 15) 
#define ACTUATOR_SEQ_LOOP_MAX		15
#define ACTUATOR_GPI_0_SEQ_ID		7
#define ACTUATOR_SCRIPT_DELAY		0xFE
#define ACTUATOR_SCRIPT_END			0xFF
#define ACTUATOR_SCRIPT_MAX			16
//! ----------------------------------------------------------
#define HAPTIC_CHIP_ID				0x07

#define SUCCESS 0
#define FAIL   -1

//! script type - a table of register address/value pairs
struct scr_type {
	uint8_t 	reg;
	uint8_t 	val;
};

//! script masks
struct scr_mask_type {
	uint8_t 	reg;
	uint8_t 	mask;
	uint8_t 	val;
};

//! device type enumeration
enum haptic_dev_t {
	LRA            = 0,
	ERM            = 1,
	ERM_COIN       = 2,
    ERM_DMA        = 3,
	LRA_DMA        = 4,
	HAPTIC_DEV_MAX,
};

/*!  DRV2605 Operating Modes
----------------------------------------------------------------------------
# Mode  Description
----------------------------------------------------------------------------
INACTIVE_MODE  = 0 Inactive- System waits in IDLE or STANDBY state based on STANDBY_EN setting
STREAM_MODE	   = 1 Direct register override - Playback streaming; input written to OVERRIDE_VAL
PWM_MODE	   = 2 Pulse width modulated - Playback streaming from PWM data input source on pin GPI_0/PWM
REGISTER_MODE  = 3 Register triggered waveform memory - Playback from Waveform Memory triggered only by write to SEQ_START
GPIO_MODE	   = 4 Edge triggered waveform memory - Playback from Waveform Memory triggered by GPIs or via write to SEQ_START
AUDIO_MODE     = 5 Audio Input to Vibration Mode 
DIAG_MODE      = 6 Daagnostic Mode
CALIBRATE_MODE = 7 Auto Calibrate Mode
------------------
SLEEP_MODE     = 8 SLEEP Prepare for lowest-power mode (Inactive+)
----------------------------------------------------------------------------
*/
//! operating modes enumeration
enum op_mode {
	INACTIVE_MODE  = 0,
	STREAM_MODE	   = 1,
	PWM_MODE	   = 2,
	REGISTER_MODE  = 3,
	GPIO_MODE	   = 4,
	AUDIO_MODE     = 5,
	DIAG_MODE      = 6,
	CALIBRATE_MODE = 7,
	SLEEP_MODE     = 8,
	HAPTIC_MODE_MAX,
};

//! gpio modes enumeration
enum gpi_modes {
	SINGLE_PTN	= 0,
	HAPTIC_GPI_MODE_MAX,
};

//! gpio polarity enumeration
enum gpi_polarity {
	RISING_EDGE		= 0,
	FALLING_EDGE	= 1,
	BOTH_EDGE		= 2,
	LEVEL_HIGH      = 3,
	LEVEL_LOW       = 4,
	HAPTIC_GPI_POLARITY_MAX,
};

//! gpio control structure
struct gpi_ctl {
	uint8_t seq_id;
	uint8_t mode;
	uint8_t polarity;
};

//! actuator description structure
struct haptic_driver {
	int     dev_effect = 0;
	int		dev_effects_max = 0;
	int     dev_script = 0;
	int		dev_scripts_max = 0;
	uint8_t dev_state = 0;
	uint8_t dev_type = ACTUATOR_HAPTIC_DEV;
	uint8_t op_mode = ACTUATOR_OP_MODE;
	uint8_t bemf_sense_en = ACTUATOR_BEMF_SENS_EN;
	uint8_t freq_track_en = ACTUATOR_FREQ_TRACK_EN;
	uint8_t acc_en = ACTUATOR_ACC_EN;
	uint8_t rapid_stop_en = ACTUATOR_RAPID_STOP_EN;
	uint8_t amp_pid_en = ACTUATOR_AMP_PID_EN;
};


class Haptic_DRV2605 {
 public:
  Haptic_DRV2605(void);
  Haptic_DRV2605(int8_t gp0_pin);
  int begin(void);  
  int readReg(uint8_t reg);
  int writeReg(uint8_t reg, uint8_t val);
  int writeRegBits(uint8_t reg, uint8_t mask, uint8_t bits);
  int writeRegBulk(uint8_t reg, uint8_t *dada, uint8_t size);
  int writeRegScript(const struct scr_type script[]);
  int writeWaveform(uint8_t reg, uint8_t *wave, uint8_t size);
  int readWaveform(uint8_t reg, uint8_t *wave, uint8_t size);
  int setWaveform(uint8_t slot, uint8_t wave);
  int setWaveformLib(uint8_t lib);
  int setScript(int num);
  int playScript(int num);
  int getScripts(void);
  int getDeviceID(void);
  int setMode(enum op_mode mode);
  int setRealtimeValue(uint8_t rtp);
  int setActuatorType(enum haptic_dev_t type);
  int go(void);
  int stop(void);
 private:
  uint8_t Haptic_I2C_Address = DRV2605_I2C_ADDR;
  int8_t _gp0_pin;
};

