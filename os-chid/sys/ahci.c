#include <ahci.h>

void start_cmd(HBA_PORT *port);
void stop_cmd(HBA_PORT *port);
int find_cmdslot(HBA_PORT *port);

HBA_MEM *abar;
uint64_t *pages_for_ahci_start;
uint66_t *pages_for_ahci_end;
uint64_t *pages_for_ahci_start_virtual;
uint64_t *pages_for_ahci_end_virtual;

int write_interface(HBA_PORT *port, DWORD startl, DWORD starth, DWORD count, QWORD buf)
{
    port->is = 0xffff;              // Clear pending interrupt bits

    int slot = find_cmdslot(port);
    if (slot == -1)
        return 0;

    uint64_t clb_addr = 0;
    clb_addr = (((clb_addr | port->clbu) << 32) | port->clb);
    //kprintf("\n clb_addr = %p", clb_addr);
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(AHCI_KERN_BASE + clb_addr);

    cmdheader += slot;

    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(DWORD);     // Command FIS size
    cmdheader->w = 1;               // Write to device
    cmdheader->c = 1;

    // 8K bytes (16 sectors) per PRDT
    cmdheader->prdtl = 1;    // PRDT entries count

    uint64_t ctb_addr=0;
    ctb_addr =(((ctb_addr | cmdheader->ctbau)<<32)|cmdheader->ctba);
    //kprintf("\n ctb_addr = %p", ctb_addr);

    //  printf("\n Cmd Header Section Entered");
    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(AHCI_KERN_BASE + ctb_addr);
    memset((uint64_t *)(AHCI_KERN_BASE + ctb_addr), 0, sizeof(HBA_CMD_TBL));


    // 8K bytes (16 sectors) per PRDT
    // Last entry
    cmdtbl->prdt_entry[0].dba = (DWORD)(buf & 0xFFFFFFFF);
    cmdtbl->prdt_entry[0].dbau = (DWORD)((buf >> 32) & 0xFFFFFFFF);
    cmdtbl->prdt_entry[0].dbc = 4096-1;   // 512 bytes per sector
    cmdtbl->prdt_entry[0].i = 0;


    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_WRITE_DMA_EXT; /// WRITE_DMA_EXT for Write ->HK

    cmdfis->lba0 = (BYTE)startl;
    cmdfis->lba1 = (BYTE)(startl>>8);
    cmdfis->lba2 = (BYTE)(startl>>16);
    cmdfis->device = 1<<6;  // LBA mode

    cmdfis->lba3 = (BYTE)(startl>>24);
    cmdfis->lba4 = (BYTE)starth;
    cmdfis->lba5 = (BYTE)(starth>>8);

    cmdfis->countl = 2;
    cmdfis->counth = 0;

    //printf("[slot]{%d}", slot);
    port->ci = 1 << slot;    // Issue command   >>HK No command after this instruction 1 -> 1 command.
//    printf("\n[Port ci ][%d]", port->ci);

    // Wait for completion

    while (1)
    {
        // In some longer duration reads, it may be helpful to spin on the DPS bit
        // in the PxIS port field as well (1 << 5)
        if ((port->ci & (1<<slot)) == 0)
        {
            break;
        }
        if (port->is & HBA_PxIS_TFES)   // Task file error
        {
            printf("Write disk error\n");
            return 0;
        }
    }

    // Check again
    if (port->is & HBA_PxIS_TFES)
    {
        printf("Write disk error\n");
        return 0;
    }
    return 1;
}

/*
int append_at(char* buf)
{
    //Check is string more than 4096,
    char *write_str = buf;
    char *write_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE));
    //memset((uint64_t *)(write_buffer), 0, VIRT_PAGE_SIZE);
    memcpy(write_buffer,write_str,strlen(write_str));
    uint64_t write_buffer_physical =(uint64_t)(((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE) - (uint64_t)0xFFFFFFFF80000000));
    //printf("\n Writing : %s at location: %d", write_buffer,offset);
    return write_interface(&abar->ports[0], offset, 0, 1, write_buffer_physical);
}
*/
int write_disk(DWORD offset, char* buf)
{
    //Check is string more than 4096,
    char *write_str = buf;
    char *write_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE));
    //memset((uint64_t *)(write_buffer), 0, VIRT_PAGE_SIZE);
    memcpy(write_buffer,write_str,strlen(write_str));
    //memcpy(write_buffer,write_str,4096);
    uint64_t write_buffer_physical =(uint64_t)(((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE) - (uint64_t)0xFFFFFFFF80000000));
    //printf("\n Writing : %s at location: %d", write_buffer,offset);
    return write_interface(&abar->ports[0], offset, 0, 1, write_buffer_physical);
}

int write_disk2(DWORD offset, char* buf)
{
    //Check is string more than 4096,
    char *write_str = buf;
    char *write_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 6*(VIRT_PAGE_SIZE));
    //memset((uint64_t *)(write_buffer), 0, VIRT_PAGE_SIZE);
    memcpy(write_buffer,write_str,strlen(write_str));
    //memcpy(write_buffer,write_str,4096);
    uint64_t write_buffer_physical =(uint64_t)(((uint64_t)pages_for_ahci_start_virtual+ 6*(VIRT_PAGE_SIZE) - (uint64_t)0xFFFFFFFF80000000));
    //printf("\n Writing : %s at location: %d", write_buffer,offset);
    return write_interface(&abar->ports[0], offset, 0, 1, write_buffer_physical);
}


int read_interface(HBA_PORT *port, DWORD startl, DWORD starth, DWORD count, QWORD buf)
{
    //printf("\n Start Read");

    port->is = 0xffff;              // Clear pending interrupt bits

    int slot = find_cmdslot(port);
    if (slot == -1)
        return 0;
    uint64_t clb_addr = 0;
    clb_addr = (((clb_addr | port->clbu) << 32) | port->clb);
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(AHCI_KERN_BASE + clb_addr);
    // printf("\n clb %x clbu %x", port->clb, port->clbu);

    cmdheader += slot;

    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(DWORD);     // Command FIS size
    cmdheader->w = 0;               // Read from device  -=> HK for write  w = 1
    cmdheader->c = 1;               // Read from device
//    cmdheader->p = 1;               // Read from device

    // 8K bytes (16 sectors) per PRDT
    cmdheader->prdtl = 1;    // PRDT entries count

    uint64_t ctb_addr=0;
    ctb_addr=(((ctb_addr | cmdheader->ctbau)<<32)|cmdheader->ctba);
    //kprintf("\n ctb_addr = %p", ctb_addr);

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(AHCI_KERN_BASE + ctb_addr);
    memset((uint64_t *)(AHCI_KERN_BASE + ctb_addr), 0, sizeof(HBA_CMD_TBL));


    // 8K bytes (16 sectors) per PRDT
    // Last entry

    //  printf("\n Cmd Header Section Entered");

    cmdtbl->prdt_entry[0].dba = (DWORD)(buf & 0xFFFFFFFF);
    cmdtbl->prdt_entry[0].dbau = (DWORD)((buf >> 32) & 0xFFFFFFFF);
    cmdtbl->prdt_entry[0].dbc = 4096-1;   // 512 bytes per sector
    cmdtbl->prdt_entry[0].i = 0;


    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_READ_DMA_EXT; /// WRITE_DMA_EXT for Write ->HK

    cmdfis->lba0 = (BYTE)startl;
    cmdfis->lba1 = (BYTE)(startl>>8);
    cmdfis->lba2 = (BYTE)(startl>>16);
    cmdfis->device = 1<<6;  // LBA mode

    cmdfis->lba3 = (BYTE)(startl>>24);
    cmdfis->lba4 = (BYTE)starth;
    cmdfis->lba5 = (BYTE)(starth>>8);

    cmdfis->countl = 1;
    cmdfis->counth = 0;

    //printf("[slot]{%d}", slot);
    port->ci = 1 << slot;    // Issue command   >>HK No command after this instruction 1 -> 1 command.
//    printf("\n[Port ci ][%d]", port->ci);

    // Wait for completion

    while (1)
    {
        // In some longer duration reads, it may be helpful to spin on the DPS bit
        // in the PxIS port field as well (1 << 5)
        if ((port->ci & (1<<slot)) == 0)
        {
            break;
        }
        if (port->is & HBA_PxIS_TFES)   // Task file error
        {
            printf("Read disk error\n");
            return 0;
        }
    }

    // Check again
    if (port->is & HBA_PxIS_TFES)
    {
        printf("Read disk error\n");
        return 0;
    }

    return 1;
}


char* read_disk(DWORD offset)
{
    //Check is string more than 4096,
 //   printf("\n=====================================");
    char *read_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 1 * (VIRT_PAGE_SIZE));
//    printf("\nread_disk: Read address: %p",read_buffer);
    //memset(read_buffer,0,VIRT_PAGE_SIZE);
    uint64_t read_buffer_physical =((uint64_t)(((uint64_t)pages_for_ahci_start_virtual+ 1 * (VIRT_PAGE_SIZE) )- (uint64_t)0xFFFFFFFF80000000));
    //printf("\nread_disk: Read address: %x",read_buffer_physical);

    //printf("\n--> BEFORE Read : %s from location: %d", read_buffer, offset);
    read_interface(&abar->ports[0], offset, 0, 1, read_buffer_physical);
    //printf("\n--> Read : %s from location: %d", read_buffer, offset);
    return read_buffer;
}

int strlen_disk(char* buf){
    int count=0;
    while(*buf++ != '\r')
      count++;
    return count;
}


int append_disk(char* buf)
{
    //Check is string more than 4096,
    //forst perform a read, then add this line in the buffer, and then perform a write.
    char *write_str = read_disk(0);
    printf("\n Existing Data at 0:%s                      ",write_str);
    printf("\n Size:%d              ",strlen_disk(write_str));
    char *write_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE));
    memcpy(write_buffer,write_str,strlen_disk(write_str));
    write_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE)+strlen(write_str));
    //while(1);
    printf("\n=>buff:%s",buf);
    //memset((uint64_t *)(write_buffer), 0, VIRT_PAGE_SIZE);
    memcpy(write_buffer,buf,strlen(buf));
    uint64_t write_buffer_physical =(uint64_t)(((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE) - (uint64_t)0xFFFFFFFF80000000));
    //printf("\n Writing : %s at location: %d", write_buffer,offset);
    write_interface(&abar->ports[0], 0, 0, 1, write_buffer_physical);
    return 1;
}
//    //Check is string more than 4096,
//    char *write_str = buf;
//    memcpy(write_buffer,write_str,strlen(write_str));


// To setup command fing a free command list slot
int find_cmdslot(HBA_PORT *port)
{
    // An empty command slot has its respective bit cleared to ï¿½0ï¿½ in both the PxCI and PxSACT registers.
    // // If not set in SACT and CI, the slot is free // Checked
    DWORD slots = (port->sact | port->ci);
    int num_of_slots= (abar->cap & 0x0f00)>>8 ; // Bit 8-12
    int i;
    for (i=0; i<num_of_slots; i++)
    {
        if ((slots&1) == 0)
            return i;
        slots >>= 1;
    }
    printf("Cannot find free command list entry\n");
    return -1;
}


// Check device type
static int check_type(HBA_PORT *port)
{
    DWORD ssts = port->ssts;

    BYTE ipm = (ssts >> 8) & 0x0F;
    BYTE det = ssts & 0x0F;

    //printf ("\n ipm %d det %d sig %d", ipm, det, port->sig);
    if (det != HBA_PORT_DET_PRESENT)    // Check drive status
        return AHCI_DEV_NULL;
    if (ipm != HBA_PORT_IPM_ACTIVE)
        return AHCI_DEV_NULL;

    switch (port->sig)
    {
        case SATA_SIG_ATAPI:
            return AHCI_DEV_SATAPI ;
        case SATA_SIG_SEMB:
            return AHCI_DEV_SEMB;
        case SATA_SIG_PM:
            return AHCI_DEV_PM;
        default:
            return AHCI_DEV_SATA;
    }

    return 0;
}
// Start command engine
void start_cmd(HBA_PORT *port)
{
    // Wait until CR (bit15) is cleared
    while (port->cmd & HBA_PxCMD_CR);

    // Set FRE (bit4) and ST (bit0)
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}

void stop_cmd(HBA_PORT *port)
{
    // Clear ST (bit0)
    port->cmd &= ~HBA_PxCMD_ST;

    // Clear FRE (bit4)
    port->cmd &= ~HBA_PxCMD_FRE;

    // Wait until FR (bit14), CR (bit15) are cleared
    while(1)
    {
        if (port->cmd & HBA_PxCMD_FR)
            continue;
        if (port->cmd & HBA_PxCMD_CR)
            continue;
        break;
    }

}

void port_rebase(HBA_PORT *port, int portno){

    int i;
    uint64_t clb_addr, fbu_addr, ctb_addr;
    stop_cmd(port);

    port->clb = (((uint64_t)pages_for_ahci_start & 0xffffffff));
//    printf("\naddress is clb= %p\n",port->clb);

    port->clbu = 0;
    port->fb =  (((uint64_t)pages_for_ahci_start + (uint64_t) ((32<<10)))& 0xffffffff);
    port->fbu =  ((((uint64_t)pages_for_ahci_start + (uint64_t) ((32<<10)))>>32)& 0xffffffff);

    port->serr = 1;   //For each implemented port, clear the PxSERR register, by writing 1 to each mplemented location
    port->is   = 0;
    port->ie   = 1;

    clb_addr = 0;
    clb_addr = ((( clb_addr | port->clbu ) << 32 ) | port->clb );
    clb_addr =  clb_addr + AHCI_KERN_BASE;
    memset((void *)clb_addr, 0, 1024);

    fbu_addr = 0;
    fbu_addr = ((( fbu_addr | port->fbu ) << 32 ) | port->fb );
    fbu_addr = fbu_addr + AHCI_KERN_BASE;
    memset((void*)fbu_addr, 0, 256);

    clb_addr = 0;
    clb_addr = ((( clb_addr | port->clbu ) << 32 ) | port->clb );
    clb_addr = ( clb_addr + AHCI_KERN_BASE);

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *) clb_addr;
    for (i = 0; i < 32; i++) {
        cmdheader[i].prdtl = 8; // 8 prdt entries per command table
        // 256 bytes per command table, 64+16+48+16*8
        // Command table offset: 40K + 8K*portno + cmdheader_index*256
        cmdheader[i].ctba =  (((uint64_t)pages_for_ahci_start + (uint64_t) ((40<<10)) + (uint64_t)((i<<8)))& 0xffffffff);
        cmdheader[i].ctbau= ((((uint64_t)pages_for_ahci_start + (uint64_t) ((40<<10)) + (uint64_t)((i<<8)))>>32)& 0xffffffff);

 		ctb_addr = 0;
        ctb_addr = ((( ctb_addr | cmdheader[i].ctbau ) << 32 ) | cmdheader[i].ctba );
        ctb_addr =  ctb_addr + AHCI_KERN_BASE;
//        printf("vaddress ctba %d = %p\n", i, addres);

        memset((void*)ctb_addr, 0, 256);
        //      memset((void*)cmdheader[i].ctba, 0, 256);
    }

    start_cmd(port);

    port->is = 0;
    port->ie = 0xffffffff;

}

void probe_port(HBA_MEM *abar_temp)
{
    // Search disk in implemented ports
    DWORD pi = abar_temp->pi;
    int i = 0;

    while (i<32)
    {
        if (pi & 1)
        {
            int dt = check_type((HBA_PORT *)&abar_temp->ports[i]);
            if (dt == AHCI_DEV_SATA)
            {
                printf("\nSATA drive found at port %d\n", i);
                abar = abar_temp;

                port_rebase(abar_temp->ports, i);
                return;
            }
            else if (dt == AHCI_DEV_SATAPI)
            {
                //printf("\nSATAPI drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_SEMB)
            {
                //printf("\nSEMB drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_PM)
            {
                //printf("\nPM drive found at port %d\n", i);
            }
            else
            {
                //printf("\nNo drive found at port %d\n", i);
            }
        }

        pi >>= 1;
        i ++;
    }
}

uint64_t* ahci_alloc_pages(uint32_t no_of_vpages)
{
    //Sohil: What is the ret addr
    uint64_t *topVirtAddr = NULL;

    topVirtAddr = (uint64_t*) sub_malloc(no_of_vpages +1 ,1);

    return topVirtAddr;
}

void init_ahci()
{

    uint32_t paddr = 0xFEBF0000;


    void * virtAddr = (void *)sub_malloc(0,1);
    // Performs a virtual to physical mapping such that the virtual address is of the form 0000 0000 xxxx xxxx
    printf("\n==> Virtual address = %x",virtAddr);
    vmmgr_map_page_after_paging(paddr, (uint64_t) virtAddr, 0);


    pages_for_ahci_start_virtual = (uint64_t*) sub_malloc(29 , 1);
    pages_for_ahci_start =(uint64_t*)((uint64_t)pages_for_ahci_start_virtual - (uint64_t)0xFFFFFFFF80000000);  // physical page start -> 32 pages assign
    abar = (HBA_MEM *)virtAddr;
   //printf("\n Capablity : %p  PI : %p VI : %p Interrupt Status : %p", abar->cap, abar->pi,abar->vs,abar->is);
    probe_port(abar);
}


