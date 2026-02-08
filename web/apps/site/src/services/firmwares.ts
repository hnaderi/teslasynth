export type Firmware = {
    id: string;
    name: string;
    version: string;
    chip: 'esp32' | 'esp32s3';
    baud: number;
    files: {
        offset: number;
        url: string;
    }[];
};

export const firmwares: Firmware[] = [
    {
        id: 'stable-esp32',
        name: 'Stable',
        version: 'v1.2.0',
        chip: 'esp32',
        baud: 921600,
        files: [
            { offset: 0x1000, url: '/fw/bootloader.bin' },
            { offset: 0x8000, url: '/fw/partition.bin' },
            { offset: 0x10000, url: '/fw/app.bin' }
        ]
    }
];
