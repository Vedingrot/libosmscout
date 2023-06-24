/*
  This source is part of the libosmscout library
  Copyright (C) 2015  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/log/LoggerImpl.h>

#include <iostream>

namespace osmscout {

  StreamLogger::StreamDestination::StreamDestination(std::ostream& stream)
  : stream(stream)
  {
    // no code
  }

  void StreamLogger::StreamDestination::Print(const std::string& value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(const std::string_view& value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(const char* value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(bool value)
  {
    stream << (value ? "true" : "false");
  }

  void StreamLogger::StreamDestination::Print(short value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(unsigned short value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(int value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(unsigned int value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(long value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(unsigned long value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(long long value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::Print(unsigned long long value)
  {
    stream << value;
  }

  void StreamLogger::StreamDestination::PrintLn()
  {
    stream << std::endl;
  }

  StreamLogger::StreamLogger(std::ostream& infoStream,
                             std::ostream& errorStream)
  : infoDestination(infoStream),
    errorDestination(errorStream)
  {
    // no code
  }

  Logger::Line StreamLogger::Log(Level level)
  {
    if (level==DEBUG || level==INFO) {
      return Line(infoDestination);
    }

    return Line(errorDestination);
  }

  ConsoleLogger::ConsoleLogger()
  : StreamLogger(std::cout,std::cerr)
  {
    // no code
  }
}

