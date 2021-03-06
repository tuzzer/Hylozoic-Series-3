#include "device_port.h"

//===========================================================================
//====== Device Port ======
//===========================================================================

//~~constructor and destructor~~
TeensyUnit::DevicePort::DevicePort(TeensyUnit& teensy_parent, const uint8_t Port_Id):
			teensy_unit(teensy_parent),
			port_id(Port_Id),
			is_all_slow(Port_Id==2 || Port_Id==5)
			
{
	
	//----- Pin assignment -----

	
	if (is_all_slow){
		output_pins[0] = teensy_unit.SPWM_pin[port_id][0];
		output_pins[1] = teensy_unit.SPWM_pin[port_id][1];
		output_pins[2] = teensy_unit.SPWM_pin[port_id][2];
		output_pins[3] = teensy_unit.SPWM_pin[port_id][3];
	}
	else{
		output_pins[0] = teensy_unit.FPWM_pin[port_id][0];
		output_pins[1] = teensy_unit.FPWM_pin[port_id][1];
		output_pins[2] = teensy_unit.SPWM_pin[port_id][0];
		output_pins[3] = teensy_unit.SPWM_pin[port_id][1];
	}
	

	analog_pins[0] = teensy_unit.Analog_pin[port_id][0];
	analog_pins[1] = teensy_unit.Analog_pin[port_id][1];
	
	acc_pin = teensy_unit.I2C_MUL_ADR[port_id];
	
}

TeensyUnit::DevicePort::~DevicePort(){
	
}

void TeensyUnit::DevicePort::init(){

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
void TeensyUnit::DevicePort::set_output_level(const uint8_t id, const uint8_t level){

	if (id >= 0 && id < 4){

		if (is_all_slow){
			noInterrupts();
			teensy_unit.spwm.setPWMFast(output_pins[id], 16*level);
			interrupts();
		}
		else{
			if (id < 2){
				analogWrite(output_pins[id], level);
			}
			else{
				noInterrupts();
				teensy_unit.spwm.setPWMFast(output_pins[id], 16*level);
				interrupts();
			}
		}
	}
}
//~~inputs~~
uint16_t TeensyUnit::DevicePort::read_analog_state(const uint8_t id){  //{IR 0, IR 1}
	if (id >= 0 && id < 2){
		return (uint16_t) analogRead(analog_pins[id]);
	}
	return 0;
}

bool TeensyUnit::DevicePort::read_acc_state(int16_t &accel_x, int16_t &accel_y, int16_t &accel_z){ // return array:{x, y, z}
	
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
void TeensyUnit::DevicePort::switchToAccel() {

	digitalWrite (teensy_unit.I2C_MUL_ADR_pin[0], (acc_pin & 1) > 0);
	digitalWrite (teensy_unit.I2C_MUL_ADR_pin[1], (acc_pin & 2) > 0);
	digitalWrite (teensy_unit.I2C_MUL_ADR_pin[2], (acc_pin & 4) > 0);
  
}

// Write a value to address register on device
void TeensyUnit::DevicePort::writeToAccel(const byte address, const byte val) {

	switchToAccel();
	noInterrupts();
	teensy_unit.Wire.beginTransmission(ACCEL); // start transmission to device 
	teensy_unit.Wire.write(address);            // send register address
	teensy_unit.Wire.write(val);                // send value to write
	teensy_unit.Wire.endTransmission(I2C_NOSTOP, I2C_TIMEOUT);         // end transmission
	interrupts();
}

