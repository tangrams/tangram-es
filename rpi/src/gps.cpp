
#include "gps.h"

#include <stdio.h>      // standard input / output functions
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <vector>       // std::vector
#include <algorithm>    // std::search
 
int openPort(const std::string& _portAddress){
    int port_fd = open(_portAddress.c_str(), O_RDONLY | O_NOCTTY);
 
    if (port_fd == -1) {
        std::cout << "Error opening serial" << std::endl;
        return -1;
    }
 
    struct termios tty;
    struct termios tty_old;
    memset(&tty, 0, sizeof tty);
 
    /* Error Handling */
    if ( tcgetattr ( port_fd, &tty ) != 0 ){
        std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        return -1;
    }
 
    /* Save old tty parameters */
    tty_old = tty;
 
    /* Set Baud Rate */
    cfsetospeed (&tty, (speed_t)B9600);
    cfsetispeed (&tty, (speed_t)B9600);
 
    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;
 
    tty.c_cflag     &=  ~CSTOPB;            // 1 stop bit
    tty.c_cflag     &=  ~CRTSCTS;           // no flow control
    tty.c_cc[VMIN]   =  1;                  // read doesn't block
    tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    // tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines
    tty.c_cflag     |=  CLOCAL; // Enable receiver
 
    /* Make raw */
    cfmakeraw(&tty);
 
    /* Flush Port, then applies attributes */
    tcflush( port_fd, TCIFLUSH );
    if ( tcsetattr ( port_fd, TCSANOW, &tty ) != 0) {
        std::cout << "Error " << errno << " from tcsetattr" << std::endl;
        return -1;
    }
 
    return port_fd;
}
 
std::string readPort(int _port_fd){
    int n = 0;
    char buf = '\0';
    std::string response = "";
    do {
        n = read( _port_fd, &buf, 1 );
        response += buf;
    } while( buf != '\r' && buf != '\n' && n > 0);
 
    if (n < 0) {
        return "ERROR";
    } else if (n == 0) {
        return "EMPTY";
    } else {
        return response;
    }
}
 
std::vector<std::string> splitString(const std::string &_source, const std::string &_delimiter = "", bool _ignoreEmpty = false) {
    std::vector<std::string> result;
    if (_delimiter.empty()) {
        result.push_back(_source);
        return result;
    }
    std::string::const_iterator substart = _source.begin(), subend;
    while (true) {
        subend = search(substart, _source.end(), _delimiter.begin(), _delimiter.end());
        std::string sub(substart, subend);
        
        if (!_ignoreEmpty || !sub.empty()) {
            result.push_back(sub);
        }
        if (subend == _source.end()) {
            break;
        }
        substart = subend + _delimiter.size();
    }
    return result;
}
 
int toInt(const std::string &_intString) {
    int x = 0;
    std::istringstream cur(_intString);
    cur >> x;
    return x;
}
 
float toFloat(const std::string &_floatString) {
    float x = 0;
    std::istringstream cur(_floatString);
    cur >> x;
    return x;
}
 
void getLocation(int _port_fd, float *lat, float *lon){
    std::string line = readPort(_port_fd);
    if (line.find("GPRMC") != std::string::npos){
        // std::cout << line << std::endl;
        std::vector<std::string> data = splitString(line,",");
 
        // If got a FIX
        if (data[2] == "A"){
            // parse the data
            *lat = 0.0f;
            *lon = 0.0f;
 
            // LATITUD
            *lat = toInt(data[3].substr(0,2));
            *lat += toFloat(data[3].substr(2))/60.0;
            if(data[4]=="S")
                *lat *= -1.0;
 
            // LONGITUD
            *lon = toInt(data[5].substr(0,3));
            *lon += toFloat(data[5].substr(3))/60.0;
            if(data[6]=="W")
                *lon *= -1.0;
        }
    }
}

void getLocation(float *lat, float *lon){
    static int fd = -1;

    if (fd<0) {
        fd = openPort("/dev/ttyAMA0");
    }

    if (fd>=0) {
        getLocation(fd,lat,lon);
    }
}