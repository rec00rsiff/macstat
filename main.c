#include <stdio.h>
#include <IOKit/IOKitLib.h>
#include <x86intrin.h>

typedef struct {
    uint32_t key;
    
    char v_l_skip[22];
    char v_l_align[2];
    
    uint32_t k_size;
    uint32_t k_type;
    char k_attr;
    char k_align[3];
    
    char s_skip[2];
    
    char sig;
    char f_align[1];
    uint32_t idx;
    unsigned char data[32];
} smc_dat_t;

void run()
{
    float temp = 0;
    
    CFMutableDictionaryRef dict = IOServiceMatching("AppleSMC");
    io_iterator_t iter;
    kern_return_t kret = IOServiceGetMatchingServices(kIOMasterPortDefault, dict, &iter);
    
    if(kret == KERN_SUCCESS)
    {
        io_object_t dev = IOIteratorNext(iter);
        if(dev)
        {
            io_connect_t cport;
            kret = IOServiceOpen(dev, mach_task_self(), 0, &cport);
            if(kret == KERN_SUCCESS)
            {
                smc_dat_t in;
                smc_dat_t out;
                
                bzero(&in, sizeof(smc_dat_t));
                bzero(&out, sizeof(smc_dat_t));
                
                in.key = 0x54433050; //TC0P //0x54433048 TC0D
                in.sig = 0x09; //info
                //index -> 2
                
                size_t in_size = sizeof(smc_dat_t);
                size_t out_size = sizeof(smc_dat_t);
                //printf("insize: %lu\n", in_size);
                //printf("sig_offset: %lu\n", offsetof(smc_dat_t, sig));
                kret = IOConnectCallStructMethod(cport, 2, &in, in_size, &out, &out_size);
                if(kret == KERN_SUCCESS)
                {
                    temp = out.k_size;
                    //__builtin_dump_struct(&out, &printf);
                    in.k_size = out.k_size;
                    in.sig = 0x05; //read
                    kret = IOConnectCallStructMethod(cport, 2, &in, in_size, &out, &out_size);
                    //sp78
                    //sbit 7(-----) 8(-----)
                    //        int      fr
                    
                    temp = (float)((out.data[0] << 8) | out.data[1]) / 256;
                }
                else
                {
                    temp = kret;
                }
            }
            kret = IOServiceClose(dev);
        }
        IOObjectRelease(dev);
    }
    IOObjectRelease(iter);
    
    uint32_t coreid;
    
    _mm_lfence();
    uint64_t start = __rdtsc();
    sleep(1);
    uint64_t end = __rdtscp(&coreid);
    _mm_lfence();
    
    uint64_t freq = end - start;
    printf("temp: %lf Celsius | freq: %llu MHz\n", temp, freq / 1000000);
}

int main(int argc, char** argv)
{
    uint8_t ff = 0;
    char optc = '?';
    while((optc = getopt(argc, argv, "f")) && optc != -1)
    {
        if(optc == 'f')
        {
            ff = 1;
        }
    }
    
    if(ff)
    {
        while(1)
        {
            run();
        }
    }
    else
    {
        run();
    }
    return 0;
}
