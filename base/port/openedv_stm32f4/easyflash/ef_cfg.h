#ifndef EF_CFG_H_
#define EF_CFG_H_

/*#define EF_USING_ENV*/
/*#define EF_USING_IAP*/
#define EF_USING_LOG

#define EF_ERASE_MIN_SIZE   (128U * 1024U)
#define EF_WRITE_GRAN       8

#define EF_START_ADDR       0x080C0000U
#define LOG_AREA_SIZE       (256U * 1024U)

#endif /* EF_CFG_H_ */
