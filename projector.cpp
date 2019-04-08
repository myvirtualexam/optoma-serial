#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <regex>
#include "projector.h"
#include "../inih/INIReader.h"

int ProjectorControl::_begin()
{
  fd = open(_device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
  tcgetattr(fd, &options);
  cfsetispeed(&options, _baud);
  cfsetospeed(&options, _baud);
  options.c_cflag |= (CS8 | CLOCAL | CREAD);
  options.c_lflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS | ICANON | ECHO | ECHOE | ISIG);
  tcsetattr(fd, TCSANOW, &options);
  tcflush(fd, TCIOFLUSH);
  //usleep(20000);
  return 0;
}

char *ProjectorControl::_frmt(std::string _cmd, std::string _arg) {
  if(_arg.length() == 0) _cmd = _cmd;
    else _cmd = _cmd.substr(0,_cmd.length()-1) + _arg;
  return (char*)_cmd.c_str();
}

std::string ProjectorControl::_write(std::string _cmd, std::string _arg)
{
  char *__cmd = _frmt(_cmd, _arg);

  // printf("cmd: %s\n", __cmd);

  int len = sizeof(__cmd);
  __cmd[len] = 0x0D;
  len++;

  tcflush(fd, TCIOFLUSH);

  std::fill(buffer, buffer+8, 0);

  int n = write(fd, __cmd, len);
  if(n != len) {
    printf("Write failed, written %i in stead of %i bytes\n", n, len);
  }
  usleep(10000);

  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  int x = 0;

  while(x<300) {
    n = read(fd, buffer, 15);
    if(n > 0) {
      buffer[16]=0;
      break;
    }
    else {
      usleep(5000);
    }
    x++;
  };
  // printf("buffer: %s length: %i\n", buffer, n);
  return std::regex_replace(std::string(buffer), std::regex("\n"), "");
}

void ProjectorControl::_end() {
  close(fd);
}

void ProjectorControl::_print(std::string _str, std::string _arg, int _res, bool _read) {
  printf("   %s %s %s %s\n", (_read==false?"- Write":"  - Read"), _str.c_str(), _arg.c_str(), (_res==0?"\e[1;32mpassed\e[0m":"\e[1;31mfailed\e[0m"));
}

std::string ProjectorControl::_cmd_read(std::string _cmd, std::string _arg, std::string _str) {
  _ret = (_write(_cmd)).c_str(); // Read current value
  //printf("ret: %s\n", _ret);
  if(strcmp(_ret,"F")==0) {
    _print(_str, _arg, 1, true);
    return "";
  }
  std::string _tmp(_ret + 2, _ret + 15);
  return _tmp;
}

int ProjectorControl::_cmd_write(std::string _cmd, std::string _arg, std::string _str) {
  _ret = (_write(_cmd, _arg)).c_str();
  //printf("input: %s\n", _ret);
  _res = strcmp(_ret,"P");
  _print(_str, _arg, _res, false);
  return _res;
}

int main(int argc, char *argv[]) {

  ProjectorControl p;

  if(argc > 1) {

    if((argc-1) % 2 != 0) {
      printf("Uneven number of arguments\n");
      return -1;
    }

    std::string own = argv[0];
    int pos = own.rfind('/');
    std::string path = own.substr(0,pos+1);

    INIReader reader(path + "mve.ini");

    if (reader.ParseError() != 0) {
        printf("Failed to load 'mve.ini'\n");
        return -1;
    }

    _device = reader.Get("projector", "device", "/dev/ttyS0");
    _baud = get_baud(reader.GetInteger("projector", "baud", 9600));
    _power_on_wait_time = reader.GetInteger("projector", "power_on_wait_time", 5);
    _time_between_commands = reader.GetInteger("projector", "time_between_commands", 100);
    _power = reader.Get("projector", "power", "");
    _mode = reader.Get("projector", "mode", "");
    _input = reader.Get("projector", "input", "");
    _speaker = reader.Get("projector", "speaker", "");
    _line = reader.Get("projector", "line", "");
    _mute = reader.Get("projector", "mute", "");
    _volume = reader.Get("projector", "volume", "");
    _format = reader.Get("projector", "format", "");
    _language = reader.Get("projector", "language", "");
    _projection = reader.Get("projector", "projection", "");

    _rd_power = reader.Get("projector", "rd_power", "");
    _rd_mode = reader.Get("projector", "rd_mode", "");
    _rd_input = reader.Get("projector", "rd_input", "");
    _rd_mute = reader.Get("projector", "rd_mute", "0");
    _rd_volume = reader.Get("projector", "rd_volume", "");
    _rd_language = reader.Get("projector", "rd_language", "");
    _rd_format = reader.Get("projector", "rd_format", "");
    _rd_projection = reader.Get("projector", "rd_projection", "");

    len = (argc-1)/2;
    char *_command[len], *_arg[len];
    bool _sent = false;
    std::string _tmp;

    for (char **pargv = argv+1; *pargv != argv[argc]; pargv++) {
      for (char *ptr = *pargv; *ptr != '\0'; ptr++) {
          if (*ptr == '-') {
            _command[i] = *pargv + 1;
            pargv++;
            _arg[i] = *pargv;
            i++;
        }
      }
    }

    p._begin();
    printf("\n");
    for(i=0;i<len;i++) {

      if(strcmp(_command[i],"power")==0) {
        _tmp=p._cmd_read(_rd_power, _arg[i], "power");
        if(_tmp!="") {
          // Compare values
          printf("   - Current power %s. ", _tmp.c_str());
          if(strcmp(_tmp.c_str(),_arg[i])==0) {
            printf("Write power %s \e[1;33munchanged\e[0m\n", _arg[i]);
            continue;
          }
          printf("\n");
        }
        usleep(_time_between_commands*1000);
        char _res=p._cmd_write(_power, _arg[i], "power");
        if(_res==0 && strcmp(_arg[i],"1")==0)
          sleep(_power_on_wait_time);
        _sent = true;
      }

      if(strcmp(_command[i],"mode")==0) {
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        // Read current value
        _tmp=p._cmd_read(_rd_mode, _arg[i], "mode");
        if(_tmp!="") {
          // Compare values
          printf("   - Current mode %s. ", _tmp.c_str());
          if(strcmp(_tmp.c_str(),_arg[i])==0) {
            printf("Write mode %s \e[1;33munchanged\e[0m\n", _arg[i]);
            continue;
          }
          printf("\n");
        }
        // Write new value
        usleep(_time_between_commands*1000);
        p._cmd_write(_mode, _arg[i], "mode");
      }

      if(strcmp(_command[i],"speaker")==0) {
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        p._cmd_write(_speaker, _arg[i], "speaker");
      }

      if(strcmp(_command[i],"line")==0) {
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        p._cmd_write(_line, _arg[i], "line");
      }

      if(strcmp(_command[i],"mute")==0) {
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        _tmp=p._cmd_read(_rd_mute, _arg[i], "mute");
        if(_tmp!="") {
          printf("   - Current mute %s. ", _tmp.c_str());
          if(strcmp(_tmp.c_str(),_arg[i])==0) {
            printf("Write mute %s \e[1;33munchanged\e[0m\n", _arg[i]);
            continue;
          }
          printf("\n");
        }
        usleep(_time_between_commands*1000);
        p._cmd_write(_mute, _arg[i], "mute");
      }

      if(strcmp(_command[i],"volume")==0) {
        /*if(_sent == true) usleep(_time_between_commands*1000);
        _tmp=p._cmd_read(_rd_volume, _arg[i], "volume");
        if(_tmp!="") {
          printf("   - Current volume %s. ", _tmp.c_str());
          if(strcmp(_tmp.c_str(),_arg[i])==0) {
            printf("Write volume %s \e[1;33munchanged\e[0m\n", _arg[i]);
            continue;
          }
          printf("\n");
        }*/
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        p._cmd_write(_volume, _arg[i], "volume");
      }

      if(strcmp(_command[i],"format")==0) {
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        _tmp=p._cmd_read(_rd_format, _arg[i], "format");
        if(_tmp!="") {
          printf("   - Current format %s. ", _tmp.c_str());
          if(strcmp(_tmp.c_str(),_arg[i])==0) {
            printf("Write format %s \e[1;33munchanged\e[0m\n", _arg[i]);
            continue;
          }
          printf("\n");
        }
        usleep(_time_between_commands*1000);
        p._cmd_write(_format, _arg[i], "format");
      }

      if(strcmp(_command[i],"language")==0) {
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        _tmp=p._cmd_read(_rd_language, _arg[i], "language");
        if(_tmp!="") {
          printf("   - Current language %s. ", _tmp.c_str());
          if(strcmp(_tmp.c_str(),_arg[i])==0) {
            printf("Write language %s \e[1;33munchanged\e[0m\n", _arg[i]);
            continue;
          }
          printf("\n");
        }
        usleep(_time_between_commands*1000);
        p._cmd_write(_language, _arg[i], "language");
      }

      if(strcmp(_command[i],"projection")==0) {
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        _tmp=p._cmd_read(_rd_projection, _arg[i], "projection");
        if(_tmp!="") {
          printf("   - Current projection %s. ", _tmp.c_str());
          if(strcmp(_tmp.c_str(),_arg[i])==0) {
            printf("Write projection %s \e[1;33munchanged\e[0m\n", _arg[i]);
            continue;
          }
          printf("\n");
        }
        usleep(_time_between_commands*1000);
        p._cmd_write(_projection, _arg[i], "projection");
      }

      if(strcmp(_command[i],"input")==0) {
        if(_sent == true) usleep(_time_between_commands*1000);
        _sent = true;
        _tmp=p._cmd_read(_rd_input, _arg[i], "input");
        if(_tmp!="") {
          printf("   - Current input %s. ", _tmp.c_str());
          if(strcmp(_tmp.c_str(),_arg[i])==0) {
            printf("Write input %s \e[1;33munchanged\e[0m\n", _arg[i]);
            continue;
          }
          printf("\n");
        }
        usleep(_time_between_commands*1000);
        p._cmd_write(_input, _arg[i], "input");
      }

    }
    printf("\n");

    p._end();
  }

  return 0;
}
