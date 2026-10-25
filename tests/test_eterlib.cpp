#include <EterLib/StdAfx.h>
#include <EterLib/Pool.h>
#include <EterLib/Mutex.h>
#include <EterLib/Thread.h>
#include <EterLib/NetAddress.h>
#include <EterLib/Ray.h>
#include <EterLib/GrpMath.h>
#include <EterLib/parser.h>
#include <EterLib/TextTag.h>
#include <EterLib/IME.h>
#include <EterLib/GrpDIB.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

struct PooledSample : public CPooledObject<PooledSample>
{
    int value = 42;
};

void test_pool_and_64bit_new()
{
    CDynamicPool<int> intPool;
    intPool.SetName("TestIntPool");
    int* p = intPool.Alloc();
    assert(p != nullptr);
    *p = 1337;
    assert(*p == 1337);
    intPool.Free(p);
    intPool.Destroy();

    PooledSample* obj = new PooledSample();
    assert(obj != nullptr);
    assert(obj->value == 42);
    delete obj;
    PooledSample::DestroySystem();

    std::printf("[PASS] test_pool_and_64bit_new\n");
}

class TestWorkerThread : public CThread
{
public:
    UINT Setup() override { return 1; }
    UINT Execute(void* arg) override
    {
        int* counter = static_cast<int*>(arg);
        *counter = 999;
        return 0;
    }
};

void test_mutex_and_thread()
{
    Mutex mtx;
    mtx.Lock();
    mtx.Unlock();

    int sharedCounter = 0;
    TestWorkerThread thread;
    int created = thread.Create(&sharedCounter);
    assert(created != 0);

    // Wait up to 1 second for thread completion
    for (int i = 0; i < 100 && sharedCounter == 0; ++i)
    {
        Sleep(10);
    }
    assert(sharedCounter == 999);

    std::printf("[PASS] test_mutex_and_thread\n");
}

void test_network_address()
{
    CNetworkAddress addr;
    bool ok = addr.Set("127.0.0.1", 13000);
    assert(ok);
    assert(addr.GetPort() == 13000);

    char ipBuf[64] = {};
    addr.GetIP(ipBuf, sizeof(ipBuf));
    assert(std::strcmp(ipBuf, "127.0.0.1") == 0);

    std::printf("[PASS] test_network_address\n");
}

void test_ray_and_grpmath()
{
    D3DXVECTOR3 start(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 dir(0.0f, 0.0f, 1.0f);
    CRay ray1(start, dir, 100.0f);
    CRay ray2;
    ray2 = ray1;

    D3DXVECTOR3 endPt;
    ray2.GetEndPoint(&endPt);
    assert(std::fabs(endPt.z - 100.0f) < 1e-4f);

    bool inTri = IsInTriangle2D(-1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    assert(inTri);

    std::printf("[PASS] test_ray_and_grpmath\n");
}

void test_text_tag_and_ime()
{
    const std::wstring tagged = L"|cffff0000Hello|r World";
    std::wstring stripped = GetTextTagOutputString(tagged.c_str(), static_cast<int>(tagged.size()));
    assert(stripped == L"Hello World");

    CIME ime;
    ime.Initialize(reinterpret_cast<HWND>(1));
    ime.SetMax(64);
    ime.SetText("Metin2Linux", 11);
    std::string out;
    int len = ime.GetText(out);
    assert(len == 11);
    assert(out == "Metin2Linux");
    ime.Uninitialize();

    std::printf("[PASS] test_text_tag_and_ime\n");
}

void test_graphic_dib()
{
    CGraphicDib dib;
    bool created = dib.Create(reinterpret_cast<HDC>(1), 32, 32);
    assert(created);
    assert(dib.GetWidth() == 32);
    assert(dib.GetHeight() == 32);
    assert(dib.GetPointer() != nullptr);
    dib.Destroy();

    std::printf("[PASS] test_graphic_dib\n");
}

} // namespace

int main()
{
    test_pool_and_64bit_new();
    test_mutex_and_thread();
    test_network_address();
    test_ray_and_grpmath();
    test_text_tag_and_ime();
    test_graphic_dib();

    std::printf("All EterLib smoke tests passed!\n");
    return 0;
}
