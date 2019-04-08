#ifndef __PROJECTORCONTROL_H__
#define __PROJECTORCONTROL_H__

#include <string>

std::string ret, _device, _power, _mode, _input, _speaker, _line, _mute, _volume, _format, _language, _projection, _rd_power, _rd_mode, _rd_input, _rd_format, _rd_projection, _rd_mute, _rd_volume, _rd_language;
int _power_on_wait_time, _time_between_commands, len, i;
speed_t _baud;
//const char *_ret;

struct termios options;

int get_baud(int baud)
{
    switch (baud) {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
    case 1000000:
        return B1000000;
    case 1152000:
        return B1152000;
    case 1500000:
        return B1500000;
    case 2000000:
        return B2000000;
    case 2500000:
        return B2500000;
    case 3000000:
        return B3000000;
    case 3500000:
        return B3500000;
    case 4000000:
        return B4000000;
    default:
        return -1;
    }
}

int main(int argc, char *argv[]);

class ProjectorControl
{
  private:
      char buffer[16] = {0};
      char _res;
      char const *_ret;

  public:
      int _begin();
      void _end();
      char *_frmt(std::string _x, std::string _arg = "");
      std::string _write(std::string _cmd, std::string _arg = "");
      void _print(std::string _str, std::string _arg="", int _res=0, bool _read=false);
      std::string _cmd_read(std::string _cmd, std::string _arg, std::string _str);
      int _cmd_write(std::string _cmd, std::string _arg, std::string _str);
      int fd;
};

extern ProjectorControl p;

#endif
