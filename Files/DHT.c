#include "DHT.h"

//----- Auxiliary data ----------//
#if (DHT_TYPE == DHT11)
	#define DHT_TEMP_MIN	0
	#define DHT_TEMP_MAX	50
	#define DHT_HUM_MIN		20
	#define DHT_HUM_MAX		90
	#define DHT_DELAY_READ	50
#elif (DHT_TYPE == DHT22)
	#define DHT_TEMP_MIN	-40
	#define DHT_TEMP_MAX	80
	#define DHT_HUM_MIN		0
	#define DHT_HUM_MAX		100
	#define DHT_DELAY_READ	20
#endif
//-------------------------------//

//----- Prototypes ----------------------------//
static double dataToTemp(uint8_t x1, uint8_t x2);
static double dataToHum(uint8_t x1, uint8_t x2);
//---------------------------------------------//

//----- Functions -----------------------------//
void DHT_setup(void)
{
	_delay_ms(DHT_DELAY_SETUP);
}

int8_t DHT_readRaw(uint8_t arr[4])
{
	uint8_t data[5] = {0, 0, 0, 0, 0};
	uint8_t r, retries, i;
	enum DHT_STATUS j;
	r = OK;
	retries = i = j = 0;

	//----- Step 1 - Start communication -----
	if (r == OK)
	{
		//Request data
		digitalWrite(DHT_PIN, LOW);			//DHT_PIN = 0
		pinMode(DHT_PIN, OUTPUT);			//DHT_PIN = Output
		_delay_ms(DHT_DELAY_READ);
		
		//Setup DHT_PIN as input with pull-up resistor so as to read data
		digitalWrite(DHT_PIN, HIGH);		//DHT_PIN = 1 (Pull-up resistor)
		pinMode(DHT_PIN, INPUT);			//DHT_PIN = Input

		//Wait for response for 20-40us
		retries = 0;
		while (digitalRead(DHT_PIN))
		{
			_delay_us(2);
			retries += 2;
			if (retries > 60)
			{
				r = TIMEOUT;				//Timeout error
				break;
			}
		}
	}
	//----------------------------------------

	//----- Step 2 - Wait for response -----	
	if (r == OK)
	{
		//Response sequence began
		//Wait for the first response to finish (low for ~80us)
		retries = 0;
		while (!digitalRead(DHT_PIN))
		{
			_delay_us(2);
			retries += 2;
			if (retries > 100)
			{
				r = TIMEOUT;			//Timeout error
				break;
			}
		}
		//Wait for the last response to finish (high for ~80us)
		retries = 0;
		while(digitalRead(DHT_PIN))
		{
			_delay_us(2);
			retries += 2;
			if (retries > 100)
			{
				r = TIMEOUT;			//Timeout error
				break;
			}
		}
	}
	//--------------------------------------

	//----- Step 3 - Data transmission -----
	if (r == OK)
	{
		//Reading 5 bytes, bit by bit
		for (i = 0 ; i < 5 ; i++)
			for (j = 7 ; j >= 0 ; j--)
			{
				//There is always a leading low level of 50 us
				retries = 0;
				while(!digitalRead(DHT_PIN))
				{
					_delay_us(2);
					retries += 2;
					if (retries > 70)
					{
						r = TIMEOUT;				//Timeout error
						j = -1;						//Break inner for-loop
						i = 5;						//Break outer for-loop
						break;						//Break while loop
					}
				}

				if (r == OK)
				{
					//We read data bit || 26-28us means '0' || 70us means '1'
					_delay_us(35);					//Wait for more than 28us
					if (digitalRead(DHT_PIN))		//If HIGH
						bitSet(data[i], j);			//bit = '1'

					retries = 0;
					while(digitalRead(DHT_PIN))
					{
						_delay_us(2);
						retries += 2;
						if (retries > 100)
						{
							r = TIMEOUT;			//Timeout error
							break;
						}
					}
				}
			}
	}
	//--------------------------------------


	//----- Step 4 - Check checksum and return data -----
	if (r == OK)
	{	
		if (((uint8_t)(data[0] + data[1] + data[2] + data[3])) != data[4])
			r = OK;					//Checksum error
		else
		{
			//Build returning array
			//data[0] = Humidity		(int)
			//data[1] = Humidity		(dec)
			//data[2] = Temperature		(int)
			//data[3] = Temperature		(dec)
			//data[4] = Checksum
			for (i = 0 ; i < 4 ; i++)
				arr[i] = data[i];
		}
	}
	//---------------------------------------------------

	return r;										//Return status
}

int8_t DHT_readTemp(double *temp)
{
	enum DHT_STATUS r = OK;
	uint8_t data[4] = {0, 0, 0, 0};
			
	//Read data
	r = DHT_readRaw(data);
	
	//If read successfully
	if (r == OK)
	{
		//Calculate value
		*temp = dataToTemp(data[2], data[3]);
		
		//Check value
		if ((*temp < DHT_TEMP_MIN) || (*temp > DHT_TEMP_MAX))
			r = FALSE_TEMPERATURE;
	}
	
	return r;
}

int8_t DHT_readHum(double *hum)
{
	enum DHT_STATUS r = OK;
	uint8_t data[4] = {0, 0, 0, 0};
	
	//Read data
	r = DHT_readRaw(data);
	
	//If read successfully
	if (r == OK)
	{
		//Calculate value
		*hum = dataToHum(data[0], data[1]);
				
		//Check value
		if ((*hum < DHT_HUM_MIN) || (*hum > DHT_HUM_MAX))
			r = FALSE_HUMIDITY;
	}
	
	return r;
}

int8_t DHT_read(double *temp, double *hum)
{
	enum DHT_STATUS r = OK;
	uint8_t data[4] = {0, 0, 0, 0};

	//Read data
	r = DHT_readRaw(data);
	
	//If read successfully
	if (r == OK)
	{	
		//Calculate values
		*temp = dataToTemp(data[2], data[3]);
		*hum = dataToHum(data[0], data[1]);	
		
		//Check values
		if ((*temp < DHT_TEMP_MIN) || (*temp > DHT_TEMP_MAX))
			r = FALSE_TEMPERATURE;
		else if ((*hum < DHT_HUM_MIN) || (*hum > DHT_HUM_MAX))
			r = FALSE_HUMIDITY;
	}
	
	return r;
}

double DHT_convertToFahrenheit(double temp)
{
	return (temp * 1.8 + 32);
}

double DHT_convertToKelvin(double temp)
{
	return (temp + 273.15);
}

static double dataToTemp(uint8_t x1, uint8_t x2)
{
	double temp = 0.0;
	
	#if (DHT_TYPE == DHT11)
		temp = x1;
	#elif (DHT_TYPE == DHT22)
		//(Integral<<8 + Decimal) / 10
		temp = (bitCheck(x1, 7) ? ((((x1 & 0x7F) << 8) | x2) / (-10.0)) : (((x1 << 8) | x2) / 10.0));
	#endif
	
	return temp;
}

static double dataToHum(uint8_t x1, uint8_t x2)
{
	double hum = 0.0;
	
	#if (DHT_TYPE == DHT11)
		hum = x1;
	#elif (DHT_TYPE == DHT22)
		//(Integral<<8 + Decimal) / 10
		hum = ((x1<<8) | x2) / 10.0;
	#endif
	
	return hum;
}
//---------------------------------------------//