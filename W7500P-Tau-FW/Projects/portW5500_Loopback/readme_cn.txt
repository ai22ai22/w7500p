��ֲ W5500 �� loopback ���̵� W7500P ���ӡ�
ʹ�õ��ǣ�ioLibrary_Driver
�ȳ�ʼ�� W7500P �� SPI1��ʹ��Ӳ����ʽ��Ϊ CS �źţ��� PC_04 ��Ϊ GPIO �����
    PC_04 -> CS
    PB_01 -> SCK
    PB_02 <- MISO
    PB_03 -> MOSI
    PC_00 -> nReset

Ȼ��ע�ᵽ ioLibrary_Driver �
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

void wizchip_select(void);
void wizchip_deselect(void);
uint8_t wizchip_read(void);
void wizchip_write(uint8_t wb);

����ֲ�����У��ο�������ʵ�ʹ������ӣ� WIZ550SR �� W5500_EVB��
��ǰһ�����������У��ҵ��˽ӿ������ľ���ʵ�֡�
�ں�һ�����������У��ҵ��� loopback �ľ���ʵ�֡�

WIZ550SR �ɲμ��� https://github.com/Wiznet/WIZ550SR.git
W5500_EVB �ɲμ���https://github.com/Wiznet/W5500_EVB.git