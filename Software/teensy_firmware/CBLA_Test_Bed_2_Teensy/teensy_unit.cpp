#include "teensy_unit.h"


//===========================================================================
//===== CONSTRUCTOR and DECONSTRUCTOR =====
//===========================================================================

TeensyUnit::TeensyUnit(): Wire(0),
	FPWM_pin {FPWM_1_pin, FPWM_2_pin, FPWM_3_pin, FPWM_4_pin, FPWM_5_pin, FPWM_6_pin},
	SPWM_pin {SPWM_1_pin, SPWM_2_pin, SPWM_3_pin, SPWM_4_pin, SPWM_5_pin, SPWM_6_pin}, 
	Analog_pin {Analog_1_pin, Analog_2_pin, Analog_3_pin, Analog_4_pin, Analog_5_pin, Analog_6_pin},
	fin0(*this, 0), 
	fin1(*this, 1),
	fin2(*this, 2),
	fin{fin0, fin1, fin2},
	light0(*this, 3),
	light1(*this, 4),
	light2(*this, 5),
	light{light0, light1, light2}
{

	
	//===============================================
	//==== pin initialization ====
	//===============================================

	uint8_t num_ports = 0;
	uint8_t num_pins = 0;
	
	//--- Teensy On-Board ---
	pinMode(indicator_led_pin, OUTPUT);

	//--- Programming Pin ---
	pinMode(PGM_DO_pin, OUTPUT);
	
	//--- FPWM pins ---
	num_ports = sizeof(FPWM_pin)/sizeof(FPWM_pin[0]);
	for (uint8_t j = 0; j<num_ports; j++){
		num_pins = sizeof(FPWM_pin[j])/sizeof(FPWM_pin[j][0]);
		for (uint8_t i = 0; i<num_pins; i++){
			pinMode(FPWM_pin[j][i], OUTPUT);
		}
	}
	
	//--- Analogue pins ---
	num_ports = sizeof(Analog_pin)/sizeof(Analog_pin[0]);
	for (uint8_t j = 0; j<num_ports; j++){
		num_pins = sizeof(Analog_pin[j])/sizeof(Analog_pin[j][0]);
		for (uint8_t i = 0; i<num_pins; i++){
			pinMode(Analog_pin[j][i], INPUT);
		}	
	}
	
	//--- Analogue settings ---
	analogReadResolution(12);
	analogReadAveraging(32);
	analogWriteResolution(8);
	analogWriteFrequency(0, 1600);
	analogWriteFrequency(1, 1600);
	analogWriteFrequency(2, 1600);
	
	//--- Slow PWM driver ----
	spwm = PWMDriver(0x40);

	//--- Multiplexer pins ---
	num_pins = sizeof(I2C_MUL_ADR_pin)/sizeof(I2C_MUL_ADR_pin[0]);
	for (uint8_t i = 0; i<num_pins; i++){
		pinMode(I2C_MUL_ADR_pin[i], OUTPUT);
	}	

	//--- I2C initialization ----
	Wire.begin(I2C_MASTER,0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);


}

TeensyUnit::~TeensyUnit(){

}

//===========================================================================
//===== INITIALIZATION =====
//===========================================================================

void TeensyUnit::init(){
	
	//----- Begin slow PWM driver ----
	spwm_init(1000);


	//---- initialize I2C accelerometer on Fin module ---
	fin0.init();
	fin1.init();
	fin2.init();

	
	//===== clear all existing messages ======
	unsigned long clearing_counter = 0;
	while (receive_msg()){
	    // this prevents the Teensy from being stuck in infinite loop
	    clearing_counter++;
	    if (clearing_counter>100){
			break;
        }
		
	}
	

}

void TeensyUnit::spwm_init(uint16_t freq){

	//----- Begin slow PWM driver ----
	noInterrupts();
	spwm.begin();
	spwm.setPWMFreq(freq);  // This is the maximum PWM frequency
	interrupts();

}

//===========================================================================
//====== COMMUNICATION ======
//===========================================================================

bool TeensyUnit::receive_msg(){

	noInterrupts();
	uint8_t byteCount = 0;
	byteCount = RawHID.recv(recv_data_buff, 0);
	interrupts();

	if (byteCount > 0) {
		
		
		// extract the front and end signatures
		byte front_signature = recv_data_buff[0];
		byte back_signature = recv_data_buff[num_incoming_byte-1];
		
		// check if it's a write-only message
		byte msg_setting = recv_data_buff[num_incoming_byte-2];

		// compose reply message
		this->compose_reply(front_signature, back_signature, msg_setting);
		send_msg();
		return true;
	}
	else{
		return false;
	}
}

void TeensyUnit::send_msg(){

	// Send a message
	noInterrupts();
	RawHID.send(send_data_buff, 10);
	interrupts();
}

uint8_t TeensyUnit::get_msg_setting(){

	return msg_setting;


}

//===========================================================================
//====== Fin Port ======
//===========================================================================
//~~constructor and destructor~~
TeensyUnit::FinPort::FinPort(TeensyUnit& teensy_parent, const uint8_t Port_Id):
			teensy_unit(teensy_parent),
			port_id(Port_Id),
			is_all_slow(Port_Id==2 || Port_Id==5)
			
{
	
	//----- Pin assignment -----

	
	if (is_all_slow){
		led_pins[0] = teensy_unit.SPWM_pin[port_id][0];
		led_pins[1] = teensy_unit.SPWM_pin[port_id][1];
		sma_pins[0] = teensy_unit.SPWM_pin[port_id][2];
		sma_pins[1] = teensy_unit.SPWM_pin[port_id][3];
	}
	else{
		led_pins[0] = teensy_unit.FPWM_pin[port_id][0];
		led_pins[1] = teensy_unit.FPWM_pin[port_id][1];
		sma_pins[0] = teensy_unit.SPWM_pin[port_id][0];
		sma_pins[1] = teensy_unit.SPWM_pin[port_id][1];
	}
	

	analog_pins[0] = teensy_unit.Analog_pin[port_id][0];
	analog_pins[1] = teensy_unit.Analog_pin[port_id][1];
	
	acc_pin = teensy_unit.I2C_MUL_ADR[port_id];
	
}

TeensyUnit::FinPort::~FinPort(){
	
}

void TeensyUnit::FinPort::init(){

	//---- initialize acceleromter ---
	writeToAccel(ACC_ACT_ADDR, ACC_ACT_VAL);  
	writeToAccel(ACC_BW_ADDR, ACC_BW_VAL);
	writeToAccel(ACC_BW_ADDR, ACC_BW_VAL);
	writeToAccel(ACC_PWRCTRL_ADDR, ACC_PWRCTRL_SLEEP | ACC_PWRCTRL_MEASURE);
	writeToAccel(ACC_INRPPT_ADDR, ACC_INRPPT_DISABLE);
	writeToAccel(ACC_DATAFORMAT_ADDR, ACC_DATAFORMAT_VALUE);
	writeToAccel(ACC_FIFO_ADDR, ACC_FIFO_VALUE);

	delay(5);

}
//~~outputs~~
void TeensyUnit::FinPort::set_sma_level(const uint8_t id, const uint8_t level){
	
	noInterrupts();
	teensy_unit.spwm.setPWMFast(sma_pins[id], 16*level);
	interrupts();
	
}
void TeensyUnit::FinPort::set_led_level(const uint8_t id, const uint8_t level){

	if (is_all_slow){
		noInterrupts();
		teensy_unit.spwm.setPWMFast(led_pins[id], 16*level);
		interrupts();
	}
	else{
		analogWrite(led_pins[id], level);
	}
}

//~~inputs~~
uint16_t TeensyUnit::FinPort::read_analog_state(const uint8_t id){  //{IR 0, IR 1}
	return (uint16_t) analogRead(analog_pins[id]);
}

bool TeensyUnit::FinPort::read_acc_state(int16_t &accel_x, int16_t &accel_y, int16_t &accel_z){ // return array:{x, y, z}
	
	noInterrupts();
	switchToAccel();

	teensy_unit.Wire.beginTransmission(ACCEL);
	teensy_unit.Wire.write(ACC_X_LSB_ADDR);
	teensy_unit.Wire.endTransmission(I2C_STOP, I2C_TIMEOUT);
	
	teensy_unit.Wire.requestFrom(ACCEL, (size_t) 6, I2C_STOP, I2C_TIMEOUT); // Read 6 bytes      
	
	uint8_t i = 0;
	byte buffer[6] = {0};
	
	//delay(5);

	while(teensy_unit.Wire.available() && i<6)
	{
		buffer[i] = teensy_unit.Wire.read();
		i++;
	}
			
	interrupts();

	accel_x = buffer[1] << 8 | buffer[0];
	accel_y = buffer[3] << 8 | buffer[2];
	accel_z = buffer[5] << 8 | buffer[4];
	
	if (i >=5)
		return true;
	else
		return false;
}

//~~accelerometer~~
// switching to the proper accel
void TeensyUnit::FinPort::switchToAccel() {

	digitalWrite (teensy_unit.I2C_MUL_ADR_pin[0], (acc_pin & 1) > 0);
	digitalWrite (teensy_unit.I2C_MUL_ADR_pin[1], (acc_pin & 2) > 0);
	digitalWrite (teensy_unit.I2C_MUL_ADR_pin[2], (acc_pin & 4) > 0);
  
}

// Write a value to address register on device
void TeensyUnit::FinPort::writeToAccel(const byte address, const byte val) {

	switchToAccel();
	noInterrupts();
	teensy_unit.Wire.beginTransmission(ACCEL); // start transmission to device 
	teensy_unit.Wire.write(address);            // send register address
	teensy_unit.Wire.write(val);                // send value to write
	teensy_unit.Wire.endTransmission(I2C_NOSTOP, I2C_TIMEOUT);         // end transmission
	interrupts();
}


//===========================================================================
//====== Light Port ======
//===========================================================================
//~~constructor and destructor~~
TeensyUnit::LightPort::LightPort(TeensyUnit& teensy_parent, const uint8_t Port_Id):
			teensy_unit(teensy_parent),
			port_id(Port_Id),
			is_slow(Port_Id==2 || Port_Id==5)
			
{

	//----- Pin assignment -----
	
	if (is_slow){
		led_pin = teensy_unit.SPWM_pin[port_id][0];
	}
	else{
		led_pin = teensy_unit.FPWM_pin[port_id][0];
	}
	
	analog_pin = teensy_unit.Analog_pin[port_id][0];

}

TeensyUnit::LightPort::~LightPort(){
	
}


//~~outputs~~
void TeensyUnit::LightPort::set_led_level(const uint8_t level){

	if (is_slow){
		noInterrupts();
		teensy_unit.spwm.setPWMFast(led_pin, 16*level);
		interrupts();
	}
	else{
		analogWrite(led_pin, level);
	}
}

//~~inputs~~
uint16_t TeensyUnit::LightPort::read_analog_state(){  
	return (uint16_t) analogRead(analog_pin);
}


				
