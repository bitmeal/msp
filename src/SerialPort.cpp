#include <SerialPort.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <iostream>

using namespace boost::asio;

SerialPort::SerialPort() : port(io) { }

SerialPort::~SerialPort() {
    port.close();
}

bool SerialPort::connect(const std::string &device) {
    this->device = device;
    try {
        port.open(device);
    }
    catch(boost::system::system_error) {
        throw NoConnection(device);
    }

    port.set_option(serial_port::baud_rate(115200));
    port.set_option(serial_port::parity(serial_port::parity::none));
    port.set_option(serial_port::character_size(serial_port::character_size(8)));
    port.set_option(serial_port::stop_bits(serial_port::stop_bits::one));
//    port.set_option(serial_port::flow_control(serial_port::flow_control::none));

    // clear buffer for new session
    clear();

    return true;
}

const std::string &SerialPort::getDevice() const {
    return device;
}

bool SerialPort::isOpen() {
    return port.is_open();
}

bool SerialPort::write(const std::vector<uint8_t> &data) {
    std::lock_guard<std::mutex> lock(lock_write);
    try {
        const std::size_t bytes_written = boost::asio::write(port, boost::asio::buffer(data.data(), data.size()));
        return (bytes_written==data.size());
    }
    catch(boost::system::system_error) {
        throw NoConnection(device);
    }
}

size_t SerialPort::read(std::vector<uint8_t> &data) {
    std::lock_guard<std::mutex> lock(lock_read);
    try {
        return boost::asio::read(port, boost::asio::buffer(data.data(), data.size()));
    }
    catch(boost::system::system_error) {
        throw NoConnection(device);
    }
}

std::vector<uint8_t> SerialPort::read(std::size_t n_bytes) {
    std::vector<uint8_t> data(n_bytes);
    read(data);
    return data;
}

uint8_t SerialPort::read() {
    return read(1).front();
}

int SerialPort::hasData() {
#if __unix__ || __APPLE__
    int available_bytes;
    if(ioctl(port.native_handle(), FIONREAD, &available_bytes)!=-1) {
        return available_bytes;
    }
    else {
        return -1;
    }
#elif _WIN32
    COMSTAT comstat;
    if (ClearCommError(port.native_handle(), NULL, &comstat) == true) {
        return comstat.cbInQue;
    }
    else {
        return -1;
    }
#else
#warning "hasData() will be unimplemented"
#endif
}

void SerialPort::clear() {
#if __unix__ || __APPLE__
    tcflush(port.native_handle(),TCIOFLUSH);
#elif _WIN32
    PurgeComm(port.native_handle(), PURGE_TXCLEAR);
#else
#warning "clear() will be unimplemented"
#endif
}
