#ifndef _SPI_H_
#define _SPI_H_

////////////////////////////////////////////////////////////////////////////////
// Class SPIPort
class SPIPort {
public:
	SPIPort(const std::string& spiDevicePath) throw();
	~SPIPort();
	
	void setMode(uint8_t mode) throw();
	void readBytes(char* rxbuffer, char* txbuffer,unsigned int length, unsigned int speedHz) throw();
	
private:
	const std::string& spiDevicePath_;
	int deviceFile_;
};

#endif
