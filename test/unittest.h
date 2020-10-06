#include "catch.hpp"

#define assertMsg(expr,msg) {bool _res_ = expr; if(!_res_) FAIL(msg); REQUIRE(_res_);}
#define assertTrue(expr) REQUIRE(expr)
#define assertFalse(expr) REQUIRE_FALSE(expr)
#define assertEquals(x,y) REQUIRE((x) == (y))
#define assertMatch(x,y) assertMsg(std::regex_match(y, std::regex(x)), x + std::string(" does not match ") + y)
#define assertIn(x,y) REQUIRE(std::find(y.begin(), y.end(), x) != y.end())
#define assertStringsEqual(x,y) REQUIRE(std::string(x) == std::string(y))
#define assertStringContains(x,y) assertMsg(std::string(y).find(x) != std::string::npos, std::string(y) + " did not contain " + std::string(x))
#define assertInRange(x,y,z) REQUIRE(x <= y); REQUIRE(y <= z)
#define assertAlmostEquals(x,y) assertInRange(x-1,y,x+1)
#define assertNull(expr) REQUIRE(expr == (void*)NULL)
#define assertNotNull(expr) REQUIRE(expr != (void*)NULL)
