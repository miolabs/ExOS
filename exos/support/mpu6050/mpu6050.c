#include "mpu6050.h"
#include <support/lpc11/i2c.h>
#include <support/i2c_hal.h>
#include <tests/support/bluetooth/ble_can.h>


//TEST  OK!

int test_i2c_mpu6050()
{
  int done = 0;
  unsigned char buffer[8] = {MPU6050_WHO_AM_I};
  hal_i2c_initialize(0, 400000); //bitrate = 400K
  hal_i2c_master_frame(0, MPU6050_ADDRESS, buffer, 1, 1); //Verify the identity
  
  if(buffer[1] == 0x68)
  {
     done = 1; //Passed
  }
  else
  {
      done = 0; //Failed
  }    

  return done;
}

//Configuration

int configuration_mpu6050()
{ 
  int done = 0;
  unsigned char test_buf[2] = {MPU6050_PWR_MGMT_1,MPU6050_ADDRESS};
  unsigned char buf[2] = {MPU6050_PWR_MGMT_1, 0x80};
  if (test_i2c_mpu6050())
  {
   
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);           //reset
    hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);      //Check register, OK!

    while(test_buf[1] != 0x40) 
    {
      hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);      //Check register, OK!
    }

    buf[0] = test_buf[0] = MPU6050_PWR_MGMT_1;  buf[1] = 0x02;        
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);           //don't sleep mode

 
    buf[0] = test_buf[0] = MPU6050_SMPLRT_DIV;  buf[1] = 0x07;     //Gyroscope Output Rate = 8kHz when the DLPF is disabled and 1Khz when the DLPF is enabled
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);           // The accelerometer output rate is 1kHz  (4.2) 

  

    buf[0] = test_buf[0] = MPU6050_CONFIG;  buf[1] = 0x00;
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);


    //GYRO_CONFIG
    buf[0] = test_buf[0] = MPU6050_GYRO_CONFIG;  buf[1] = 0x08;
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);


    //ACCEL_CONFIG
    buf[0] = test_buf[0] = MPU6050_ACCEL_CONFIG;  buf[1] = 0x08;
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);

  
    buf[0] = test_buf[0] = MPU6050_FF_THR;  buf[1] = 0x00;
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);


    buf[0] = test_buf[0] = MPU6050_FF_DUR;  buf[1] = 0x00;
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);


    buf[0] = test_buf[0] = MPU6050_MOT_THR;  buf[1] = 0x00;
    hal_i2c_master_frame(0, MPU6050_ADDRESS, buf, 2, 0);

    done = 1;
   } //conection error 
}

int check_reg_mpu6050()
{
//checking the registers
  int error = 0;
  unsigned char test_buf[2] = {MPU6050_PWR_MGMT_1,MPU6050_ADDRESS};

  test_buf[0] = MPU6050_SMPLRT_DIV;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x07) {error = 10; };
  test_buf[0] = MPU6050_CONFIG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 11; };
  test_buf[0] = MPU6050_GYRO_CONFIG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0b00001000) { error = 12; };
  test_buf[0] = MPU6050_ACCEL_CONFIG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0b00001000) { error = 13; };
  test_buf[0] = MPU6050_FF_THR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 14; };
  test_buf[0] = MPU6050_FF_DUR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 15; };
  test_buf[0] = MPU6050_MOT_THR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 16; };
  test_buf[0] = MPU6050_MOT_DUR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 17; };
  test_buf[0] = MPU6050_ZRMOT_THR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 18; };
  test_buf[0] = MPU6050_ZRMOT_DUR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 19; };
  test_buf[0] = MPU6050_FIFO_EN;
  hal_i2c_master_frame(0, MPU6050_ADDRESS,test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 20; };
  test_buf[0] = MPU6050_I2C_MST_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 21; };
  test_buf[0] = MPU6050_I2C_SLV0_ADDR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 22; };
  test_buf[0] = MPU6050_I2C_SLV0_REG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 23; };
  test_buf[0] = MPU6050_I2C_SLV0_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 24; };
  test_buf[0] = MPU6050_I2C_SLV1_ADDR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 25; };
  test_buf[0] = MPU6050_I2C_SLV1_REG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 26; };
  test_buf[0] = MPU6050_I2C_SLV1_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 27; };
  test_buf[0] = MPU6050_I2C_SLV2_ADDR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 28; };
  test_buf[0] = MPU6050_I2C_SLV2_REG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 29; };
  test_buf[0] = MPU6050_I2C_SLV2_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 30; };
  test_buf[0] = MPU6050_I2C_SLV3_ADDR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 31; };
  test_buf[0] = MPU6050_I2C_SLV3_REG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 32; };
  test_buf[0] = MPU6050_I2C_SLV3_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 33; };
  test_buf[0] = MPU6050_I2C_SLV4_ADDR;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 34; };
  test_buf[0] = MPU6050_I2C_SLV4_REG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 35; };
  test_buf[0] = MPU6050_I2C_SLV4_DO;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 36; };
  test_buf[0] = MPU6050_I2C_SLV4_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 37; };
  test_buf[0] = MPU6050_I2C_SLV4_DI;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) {error = 38; };
  test_buf[0] = MPU6050_INT_PIN_CFG;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 39; };
  test_buf[0] = MPU6050_INT_ENABLE;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 40; };
  test_buf[0] = MPU6050_I2C_SLV0_DO;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 41; };
  test_buf[0] = MPU6050_I2C_SLV1_DO;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 42; };
  test_buf[0] = MPU6050_I2C_SLV2_DO;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 43; };
  test_buf[0] = MPU6050_I2C_SLV3_DO;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 44; };
  test_buf[0] = MPU6050_I2C_MST_DELAY_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 45; };
  test_buf[0] = MPU6050_SIGNAL_PATH_RESET;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 46; };
  test_buf[0] = MPU6050_MOT_DETECT_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 47; };
  test_buf[0] = MPU6050_USER_CTRL;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 48; };
  test_buf[0] = MPU6050_PWR_MGMT_1;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x02) { error = 49; };
  test_buf[0] = MPU6050_PWR_MGMT_2;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 50; };
  test_buf[0] = MPU6050_FIFO_R_W;
  hal_i2c_master_frame(0, MPU6050_ADDRESS, test_buf, 1, 1);
  if(test_buf[1] != 0x00) { error = 51; };

  return error;
}

int get_gyro_xyz(unsigned char *frame)
{
  unsigned char buffer[7] = {MPU6050_GYRO_XOUT_H};
  hal_i2c_master_frame(0, MPU6050_ADDRESS, buffer, 1, 6);

  signed short gyro_x = (( buffer[1] << 8 ) | buffer[2] );
  signed short gyro_y = (( buffer[3] << 8 ) | buffer[4] );
  signed short gyro_z = (( buffer[5] << 8 ) | buffer[6] ); 
  
  for (int i = 0; i < 6; i++)
    frame[i]= buffer[i+1];

  return 1;
}


int get_acel_xyz(unsigned char *frame)
{
  unsigned char buffer[7] = {MPU6050_ACCEL_XOUT_H};
  hal_i2c_master_frame(0, MPU6050_ADDRESS, buffer, 1, 6);  

  signed short acel_x = (( buffer[1] << 8 ) | buffer[2] );
  signed short acel_y = (( buffer[3] << 8 ) | buffer[4] );
  signed short acel_z = (( buffer[5] << 8 ) | buffer[6] );

  //Para la prueba con blue, envio los valores sin separar
  for (int i = 0; i < 6; i++)
    frame[i]= buffer[i+1];

  return 1;
}

void get_temp()
{
  unsigned char buffer[3] = {MPU6050_TEMP_OUT_H};
  hal_i2c_master_frame(0, MPU6050_ADDRESS, buffer, 1, 2);
  signed short gtemp = ((signed short)(( buffer[1] << 8 ) | buffer[2] ) / 340);
}

int ble_acel_read_data(BLE_ACEL_REPORT_DATA *data)
{        
        unsigned char acel_xyz[6];        
	int done_acel = get_acel_xyz(&acel_xyz);
	if (done_acel)
	{
		for (int i = 0; i < 6; i++) 
			data->Data[i] = acel_xyz[i];
	}
        return (done_acel);
}

int ble_gyro_read_data(BLE_GYRO_REPORT_DATA *data)
{        
        unsigned char gyro_xyz[6];
	int done_gyro = get_gyro_xyz(&gyro_xyz);
	if (done_gyro)
        {
                for (int i = 0; i < 6; i++) 
			data->Data[i] = gyro_xyz[i];
	}
        return (done_gyro);
}

unsigned char ble_mpu6050_transmit(BLE_MPU_TRANSMIT_DATA *data)
{
    return data->Data[0];
}