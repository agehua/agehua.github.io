---
layout: post
title: BLE 介绍和使用
category: accumulation
tags:
  - BLE
keywords: BLE
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/La%20Mousme%2C%20Sitting%202.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/La%20Mousme%2C%20Sitting%202.jpg
toc: true
---

### BLE
蓝牙低功耗（Bluetooth Low Energy, BLE）的配对过程涉及设备之间的身份验证和密钥交换，以确保通信的安全性。BLE 的配对过程可以分为以下几个阶段：
<!--more-->
配对发起：一台设备（通常是中央设备，例如手机）发起配对请求。
密钥生成与交换：设备之间通过不同的配对方法生成和交换密钥。
加密通信：使用共享密钥加密通信。

在 BLE 中，配对过程的具体方法取决于设备的输入/输出能力和所选的配对方法（例如，Just Works、Passkey Entry、Numeric Comparison 等）。

以下是使用 Java 开发 Android 应用时，处理 BLE 配对的代码示例和步骤。

#### 1. 启动 BLE 扫描并连接设备

在 Android 中，使用 BluetoothAdapter 和 BluetoothLeScanner 扫描 BLE 设备，并连接到目标设备。

~~~ java
BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
BluetoothLeScanner bluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner();

ScanCallback scanCallback = new ScanCallback() {
    @Override
    public void onScanResult(int callbackType, ScanResult result) {
        super.onScanResult(callbackType, result);
        BluetoothDevice device = result.getDevice();
        // 发现目标设备后，停止扫描并连接
        if (device.getName() != null && device.getName().equals("TargetDeviceName")) {
            bluetoothLeScanner.stopScan(this);
            connectToDevice(device);
        }
        // 获取从设备的 MAC 地址
        String macAddress = device.getAddress();
        Log.d("BLE", "Found device MAC: " + macAddress);
    }
};

private void startScan() {
    bluetoothLeScanner.startScan(scanCallback);
}

private void connectToDevice(BluetoothDevice device) {
    device.connectGatt(this, false, gattCallback);
}
~~~

#### 2. BLE 配对

BLE 配对的核心是处理设备的配对请求和响应。在 Android 中，配对过程由系统管理，但你可以通过监听 BroadcastReceiver 捕获配对事件。

~~~ java
// 注册 BroadcastReceiver 监听配对请求
private void registerPairingReceiver() {
    IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_PAIRING_REQUEST);
    registerReceiver(pairingReceiver, filter);
}

private final BroadcastReceiver pairingReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (BluetoothDevice.ACTION_PAIRING_REQUEST.equals(action)) {
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            int pairingVariant = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_VARIANT, BluetoothDevice.ERROR);

            if (pairingVariant == BluetoothDevice.PAIRING_VARIANT_PIN) {
                // 如果需要输入 PIN 码
                device.setPin("123456".getBytes());
                device.setPairingConfirmation(true);
            } else if (pairingVariant == BluetoothDevice.PAIRING_VARIANT_PASSKEY_CONFIRMATION) {
                // 如果需要确认 Passkey
                int passkey = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_KEY, -1);
                Log.d("BLE", "Passkey: " + passkey);
                device.setPairingConfirmation(true);
            }
        }
    }
};
~~~

#### 3. 处理 GATT 连接和加密

在配对完成后，可以使用 BluetoothGatt 进行通信。配对完成后，通信会自动加密。

~~~ java
private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
    @Override
    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
        if (newState == BluetoothProfile.STATE_CONNECTED) {
            Log.d("BLE", "Connected to GATT server.");
            gatt.discoverServices();
        } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
            Log.d("BLE", "Disconnected from GATT server.");
        }
    }

    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
        if (status == BluetoothGatt.GATT_SUCCESS) {
            // 获取目标服务和特征值
            BluetoothGattService service = gatt.getService(UUID.fromString("0000180d-0000-1000-8000-00805f9b34fb")); // 替换为目标服务 UUID
            if (service != null) {
                BluetoothGattCharacteristic characteristic = service.getCharacteristic(UUID.fromString("00002a37-0000-1000-8000-00805f9b34fb")); // 替换为目标特征值 UUID
                if (characteristic != null) {
                    // 订阅通知
                    gatt.setCharacteristicNotification(characteristic, true);

                    // 配置特征值的通知描述符（如果需要）
                    BluetoothGattDescriptor descriptor = characteristic.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
                    if (descriptor != null) {
                        descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                        gatt.writeDescriptor(descriptor);
                    }
                }
            }
        }
    }

    @Override
    public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
        // 当特征值的通知数据发生变化时触发
        byte[] data = characteristic.getValue();
        Log.d("BLE", "Received data: " + Arrays.toString(data));
    }
};
~~~

#### 4. 释放资源

在完成配对和通信后，记得释放资源。

~~~ java
@Override
protected void onDestroy() {
    super.onDestroy();
    unregisterReceiver(pairingReceiver);
    if (gatt != null) {
        gatt.close();
        gatt = null;
    }
}
~~~

#### 5.连接已配对设备

当设备已配对，下次连接时无需重新配对，直接通过设备的 MAC 地址进行连接：

~~~ java
private void connectToPairedDevice(String macAddress) {
    BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    BluetoothDevice device = bluetoothAdapter.getRemoteDevice(macAddress);

    if (device != null) {
        device.connectGatt(this, false, gattCallback);
    } else {
        Log.d("BLE", "Device not found!");
    }
}
~~~

#### 从设备发送数据——通知（Notification）

其实就是 onCharacteristicChanged 回调

### CRC16

CRC16 是一种 循环冗余校验（Cyclic Redundancy Check, CRC） 算法，用于检测数据传输或存储过程中的错误。它是一种基于二进制除法的校验技术，能够有效地检测数据在传输或存储中的错误（例如比特翻转或数据损坏）。

CRC16 的 "16" 指的是校验码的长度为 16 位（二进制），即生成的校验值是一个 16 位的整数，范围是 0x0000 到 0xFFFF

#### CRC 的基本原理

CRC 的核心思想是将数据视为一个二进制多项式，并通过与一个固定的生成多项式（Generator Polynomial）进行二进制除法来生成校验码。

主要步骤：

- 将要校验的数据视为一个二进制数（或多项式）。
- 使用一个固定的生成多项式对数据进行二进制除法。
- 余数即为生成的 CRC 校验码。
- 在数据传输时，将数据和 CRC 校验码一起发送。
- 接收方对接收到的数据（包括校验码）再次进行 CRC 计算。如果结果为 0，则说明数据未发生错误；否则说明数据有误。

#### CRC16 的特点

校验码长度：16 位（2 字节）。
生成多项式：CRC16 使用的生成多项式可以有多种常见的标准，例如：
CRC-16-IBM（常用）：x^16 + x^15 + x^2 + 1，对应多项式值 0x8005。
CRC-16-CCITT：x^16 + x^12 + x^5 + 1，对应多项式值 0x1021。
CRC-16-MODBUS：与 CRC-16-IBM 相同，但初始值和字节顺序不同。
其他变种：如 CRC-16-ANSI、CRC-16-XMODEM 等。

#### CRC16-CCITT 的生成多项式

CRC16-CCITT 的生成多项式是：

~~~ c
x^16 + x^12 + x^5 + 1
~~~

如何从多项式转为十六进制？
每个 x^n 表示多项式中第 n 位是 1，其余位是 0。

把多项式转成二进制形式：

- x^16 → 第 16 位是 1。
- x^12 → 第 12 位是 1。
- x^5 → 第 5 位是 1。
- 1 → 第 0 位是 1。
整体的二进制表示是：

~~~ c
1 0000 0010 0001
~~~

（从高位到低位，16 位）

转换为十六进制表示：

~~~ c
0x1021
~~~

因此，0x1021 是生成多项式 x^16 + x^12 + x^5 + 1 的十六进制表示

#### 示例计算

假设我们要计算字符串 "1234" 的 CRC16 值（使用 CRC16-CCITT 标准，生成多项式 0x1021，初始值 0xFFFF）。

- 数据：
字符串 "1234" 的 ASCII 编码是 [0x31, 0x32, 0x33, 0x34]。
- 步骤：
  - 初始化 CRC 值：
    - 初始值：0xFFFF。
  - 逐字节处理：
    - 第一个字节 0x31：
      - 将 0x31 左移 8 位，与当前 CRC 值 0xFFFF 的高 8 位异或：

            ~~~ c
            CRC = 0xFFFF ^ (0x31 << 8) = 0xCEFF
            ~~~

    - 根据生成多项式 0x1021，逐位处理 CRC 的 16 位。
  - 重复以上步骤，处理字节 0x32、0x33 和 0x34。
- 最终结果：
经过计算，最终 CRC 值为 0x29B1。
