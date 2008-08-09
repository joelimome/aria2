#include "ServerStatMan.h"
#include "ServerStat.h"
#include "Exception.h"
#include "Util.h"
#include <iostream>
#include <sstream>
#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {

class ServerStatManTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ServerStatManTest);
  CPPUNIT_TEST(testAddAndFind);
  CPPUNIT_TEST(testSave);
  CPPUNIT_TEST_SUITE_END();
public:
  void setUp() {}

  void tearDown() {}

  void testAddAndFind();

  void testSave();
};


CPPUNIT_TEST_SUITE_REGISTRATION(ServerStatManTest);

void ServerStatManTest::testAddAndFind()
{
  SharedHandle<ServerStat> localhost_http(new ServerStat("localhost", "http"));
  SharedHandle<ServerStat> localhost_ftp(new ServerStat("localhost", "ftp"));
  SharedHandle<ServerStat> mirror(new ServerStat("mirror", "http"));

  ServerStatMan ssm;
  CPPUNIT_ASSERT(ssm.add(localhost_http));
  CPPUNIT_ASSERT(!ssm.add(localhost_http));
  CPPUNIT_ASSERT(ssm.add(localhost_ftp));
  CPPUNIT_ASSERT(ssm.add(mirror));

  {
    SharedHandle<ServerStat> r = ssm.find("localhost", "http");
    CPPUNIT_ASSERT(!r.isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("localhost"), r->getHostname());
    CPPUNIT_ASSERT_EQUAL(std::string("http"), r->getProtocol());
  }
  {
    SharedHandle<ServerStat> r = ssm.find("mirror", "ftp");
    CPPUNIT_ASSERT(r.isNull());
  }
}

void ServerStatManTest::testSave()
{
  SharedHandle<ServerStat> localhost_http(new ServerStat("localhost", "http"));
  localhost_http->setDownloadSpeed(25000);
  localhost_http->setLastUpdated(Time(1210000000));
  SharedHandle<ServerStat> localhost_ftp(new ServerStat("localhost", "ftp"));
  localhost_ftp->setDownloadSpeed(30000);
  localhost_ftp->setLastUpdated(Time(1210000001));
  SharedHandle<ServerStat> mirror(new ServerStat("mirror", "http"));
  mirror->setDownloadSpeed(0);
  mirror->setError();
  mirror->setLastUpdated(Time(1210000002));

  ServerStatMan ssm;
  CPPUNIT_ASSERT(ssm.add(localhost_http));
  CPPUNIT_ASSERT(ssm.add(localhost_ftp));
  CPPUNIT_ASSERT(ssm.add(mirror));

  std::stringstream ss;
  ssm.save(ss);
  std::string out = ss.str();
  CPPUNIT_ASSERT_EQUAL
    (std::string
     ("host=localhost, protocol=ftp, dl_speed=30000, last_updated=1210000001, status=OK\n"
      "host=localhost, protocol=http, dl_speed=25000, last_updated=1210000000, status=OK\n"
      "host=mirror, protocol=http, dl_speed=0, last_updated=1210000002, status=ERROR\n"),
     out);			   
}

} // namespace aria2