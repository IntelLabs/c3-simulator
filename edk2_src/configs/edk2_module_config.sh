#!/usr/bin/env bash
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

disabled_modules=()
module_names=(
    # AcpiPlatform
    AcpiPlatform_cb933912-df8f-4305-b1f9-7b44fa11395c
    AcpiPlatform_FC90EB7A-3E0A-483C-A26C-484D36593FF4
    AcpiTableDxe
    ArpDxe
    AtaAtapiPassThruDxe
    AtaBusDxe
    BdsDxe
    BoardBdsHookDxe
    BootGraphicsResourceTableDxe
    BootManagerMenuApp
    BootScriptExecutorDxe
    CapsuleRuntimeDxe
    ConPlatformDxe
    ConSplitterDxe
    CpuDxe
    CpuIo2Dxe
    CpuIo2Smm
    CpuS3DataDxe
    DevicePathDxe
    Dhcp4Dxe
    Dhcp6Dxe
    DiskIoDxe
    DisplayEngine
    DnsDxe
    DpcDxe
    DriverHealthManagerDxe
    DxeCore
    EbcDxe
    EhciDxe
    EnglishDxe
    Example1_App
    Fat
    FirmwarePerformanceDxe
    FirmwarePerformanceSmm
    GraphicsConsoleDxe
    HelloWorld
    HiiDatabase
    HpetTimerDxe
    HttpBootDxe
    HttpDxe
    HttpUtilitiesDxe
    Ip4Dxe
    Ip6Dxe
    LegacySioDxe
    Metronome
    MnpDxe
    MonotonicCounterRuntimeDxe
    Mtftp4Dxe
    Mtftp6Dxe
    NullMemoryTestDxe
    NvmExpressDxe
    PartitionDxe
    PcdDxe
    PchSpiSmm
    PciBusDxe
    PciHostBridgeDxe
    PcRtc
    PiSmmCore
    PiSmmCpuDxeSmm
    PiSmmIpl
    PlatformInitDxe
    PlatformInitSmm
    PrintDxe
    Ps2KeyboardDxe
    QemuVideoDxe
    ReportStatusCodeRouterRuntimeDxe
    ReportStatusCodeRouterRuntimeDxe
    ReportStatusCodeRouterSmm
    ResetSystemRuntimeDxe
    RuntimeDxe
    S3SaveStateDxe
    SataController
    ScsiBus
    ScsiDisk
    SecurityStubDxe
    SerialDxe
    SetupBrowser
    Shell
    SimicsAgent
    SimicsDxe
    SmbiosBasic
    SmbiosPlatformDxe
    SmmAccess2Dxe
    SmmCommunicationBufferDxe
    SmmControl2Dxe
    SmmFaultTolerantWriteDxe
    SmmLockBox
    SnpDxe
    SpiFvbServiceSmm
    StatusCodeHandlerRuntimeDxe
    StatusCodeHandlerSmm
    TcpDxe
    TerminalDxe
    Udp4Dxe
    Udp6Dxe
    UefiPxeBcDxe
    UhciDxe
    UiApp
    UsbBusDxe
    UsbKbDxe
    UsbMassStorageDxe
    VariableRuntimeDxe
    VariableSmm
    VariableSmmRuntimeDxe
    VlanConfigDxe
    WatchdogTimer
    XhciDxe
)

# Don't mess with Simics Agent
disabled_modules+=( SimicsAgent )

# Crashes at UEFI shell, access outside GSRIP power-slot from UEFI module
disabled_modules+=(
    Shell                           # ICV escape
)

# ICV mismatch on module load, CA write to non-CA memory
disabled_modules+=(
    PiSmmCpuDxeSmm
)

# Causes system to halt (without error messages) on Linux boot
disabled_modules+=(
    StatusCodeHandlerRuntimeDxe
)

# ICV mismatch during Linux boot from PeCoffLoaderRelocateImageForRuntime
disabled_modules+=(
    PiSmmIpl
    ReportStatusCodeRouterRuntimeDxe
    ResetSystemRuntimeDxe
    SmmControl2Dxe
    VariableSmmRuntimeDxe
)
