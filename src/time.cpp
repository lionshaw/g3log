/** ==========================================================================
* 2012 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
* with no warranties. This code is yours to share, use and modify with no
* strings attached and no restrictions or obligations.
* 
* For more information see g3log/LICENSE or refer refer to http://unlicense.org
* ============================================================================*/

#include "g3log/time.hpp"

#include <sstream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <cassert>
#include <iomanip>


namespace g3 {
   namespace internal {
      // This mimics the original "std::put_time(const std::tm* tmb, const charT* fmt)"
      // This is needed since latest version (at time of writing) of gcc4.7 does not implement this library function yet.
      // return value is SIMPLIFIED to only return a std::string

      std::string put_time(const struct tm *tmb, const char *c_time_format) {
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)) && !defined(__MINGW32__)
         std::ostringstream oss;
         oss.fill('0');
         // BOGUS hack done for VS2012: C++11 non-conformant since it SHOULD take a "const struct tm*  "
         oss << std::put_time(const_cast<struct tm *> (tmb), c_time_format);
         return oss.str();
#else    // LINUX
         const size_t size = 1024;
         char buffer[size]; // IMPORTANT: check now and then for when gcc will implement std::put_time.
         //                    ... also ... This is way more buffer space then we need

         auto success = std::strftime(buffer, size, c_time_format, tmb);
         // TODO: incorrect code
         if (0 == success)
         {
            assert((0 != success) && "strftime fails with illegal formatting");
            return c_time_format;
         }

         return buffer;
#endif
      }
   } // internal
} // g3



namespace g3 {

   std::time_t systemtime_now() {
      system_time_point system_now = std::chrono::system_clock::now();
      return std::chrono::system_clock::to_time_t(system_now);
   }

   tm localtime(const std::time_t &time) {
      struct tm tm_snapshot;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) && !defined(__GNUC__))
      localtime_s(&tm_snapshot, &time); // windsows
#else
      localtime_r(&time, &tm_snapshot); // POSIX
#endif
      return tm_snapshot;
   }

   /// returns a std::string with content of time_t as localtime formatted by input format string
   /// * format string must conform to std::put_time
   /// This is similar to std::put_time(std::localtime(std::time_t*), time_format.c_str());

   namespace {
      const std::string kIdentifier   = "%f";
      const ulong       kZeroes[3][2] = {{3, 1000000}, {6, 1000}, {9, 1}};
   }

   std::string localtime_formatted(const timespec &time_snapshot, const std::string &time_format) {
      auto        format_buffer = time_format;  // copying format string to a separate buffer
      std::string value_str[3];                 // creating an array of sec fractional parts

      // iterating through every "%f" instance in the format string
      for(size_t pos = 0; (pos = format_buffer.find(kIdentifier, pos)) != std::string::npos; pos += kIdentifier.size()) {

         // figuring out whether this is nano, micro or milli identifier
         ushort index = 2;
         char   ch    = (format_buffer.size() > pos + kIdentifier.size() ? format_buffer.at(pos + kIdentifier.size()) : '\0');
         switch (ch) {
            case '3': index = 0;    break;
            case '6': index = 1;    break;
            case '9': index = 2;    break;
            default : ch    = '\0'; break;
         }

         // creating sec fractional value string if required
         if (value_str[index].empty()) {
            value_str[index] = std::to_string((long)std::round((long double)time_snapshot.tv_nsec / kZeroes[index][1]));
            value_str[index] = std::string(kZeroes[index][0] - value_str[index].length(), '0') + value_str[index];
         }

         // replacing "%f[3|6|9]" with sec fractional part value
         format_buffer.replace(pos, kIdentifier.size() + (ch == '\0' ? 0 : 1), value_str[index]);
      }

      // using new format string that might contain sec fractional part
      std::tm t = localtime(time_snapshot.tv_sec);
      return g3::internal::put_time(&t, format_buffer.c_str());
   }

   std::string localtime_formatted(const std::time_t &time_snapshot, const std::string &time_format) {
      std::tm t = localtime(time_snapshot); // could be const, but cannot due to VS2012 is non conformant for C++11's std::put_time (see above)
      return g3::internal::put_time(&t, time_format.c_str()); // format example: //"%Y/%m/%d %H:%M:%S");
   }
} // g3
