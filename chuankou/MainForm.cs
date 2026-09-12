using System;
using System.Diagnostics;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SerialTimeSync
{
    public sealed class MainForm : Form
    {
        private const int BaudRate = 115200;
        private const byte FrameHead = 0x01;
        private const byte FrameTail = 0xAA;
        private const byte Ack = 0x06;
        private static readonly string[] MelodyNames =
        {
            "音乐1 · 欢乐颂", "音乐2 · 天空之城",
            "音乐3 · 曹操"
        };

        private readonly ComboBox portCombo = new ComboBox();
        private readonly Button refreshButton = new Button();
        private readonly Button connectButton = new Button();
        private readonly Button autoSyncButton = new Button();
        private readonly Button manualSyncButton = new Button();
        private readonly Button fillNowButton = new Button();
        private readonly Label pcClockLabel = new Label();
        private readonly Label statusLabel = new Label();
        private readonly TextBox logBox = new TextBox();
        private readonly NumericUpDown hourInput = new NumericUpDown();
        private readonly NumericUpDown minuteInput = new NumericUpDown();
        private readonly NumericUpDown secondInput = new NumericUpDown();
        private readonly NumericUpDown[] alarmHourInputs =
        {
            new NumericUpDown(), new NumericUpDown(), new NumericUpDown()
        };
        private readonly NumericUpDown[] alarmMinuteInputs =
        {
            new NumericUpDown(), new NumericUpDown(), new NumericUpDown()
        };
        private readonly ComboBox[] alarmMusicCombos =
        {
            new ComboBox(), new ComboBox(), new ComboBox()
        };
        private readonly Button[] alarmSyncButtons =
        {
            new Button(), new Button(), new Button()
        };
        private readonly CheckBox[] alarmEnableChecks =
        {
            new CheckBox(), new CheckBox(), new CheckBox()
        };
        private readonly Button[] alarmSwitchButtons =
        {
            new Button(), new Button(), new Button()
        };
        private readonly TabControl tabs = new TabControl();
        private readonly TabPage temperaturePage = new TabPage("温度显示");
        private readonly TabPage screenPage = new TabPage("息屏设置");
        private readonly CheckBox screenAutoOffCheck = new CheckBox();
        private readonly Button screenSaveButton = new Button();
        private readonly Label temperatureValueLabel = new Label();
        private readonly Label temperatureUpdatedLabel = new Label();
        private readonly CheckBox temperatureAutoCheck = new CheckBox();
        private readonly Button temperatureRefreshButton = new Button();
        private readonly Timer clockTimer = new Timer();
        private readonly object serialLock = new object();

        private SerialPort serialPort;
        private bool busy;
        private DateTime nextTemperaturePollUtc = DateTime.MinValue;

        public MainForm()
        {
            Text = "单片机串口控制助手";
            Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath);
            ClientSize = new Size(680, 560);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            Font = new Font("Microsoft YaHei UI", 10F);

            BuildInterface();
            RefreshPorts();
            FillCurrentTime();
            UpdatePcClock();

            clockTimer.Interval = 200;
            clockTimer.Tick += ClockTimerTick;
            clockTimer.Start();
        }

        private void BuildInterface()
        {
            Label title = new Label();
            title.Text = "单片机串口控制助手";
            title.Font = new Font("Microsoft YaHei UI", 18F, FontStyle.Bold);
            title.AutoSize = true;
            title.Location = new Point(22, 18);
            Controls.Add(title);

            GroupBox portGroup = CreateGroup(this, "串口连接", 20, 62, 640, 94);
            AddLabel(portGroup, "串口：", 18, 34, 55);
            portCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            portCombo.Location = new Point(72, 30);
            portCombo.Size = new Size(180, 30);
            portGroup.Controls.Add(portCombo);

            refreshButton.Text = "刷新";
            refreshButton.Location = new Point(265, 29);
            refreshButton.Size = new Size(82, 32);
            refreshButton.Click += delegate { RefreshPorts(); };
            portGroup.Controls.Add(refreshButton);

            connectButton.Text = "连接";
            connectButton.Location = new Point(359, 29);
            connectButton.Size = new Size(100, 32);
            connectButton.Click += ConnectButtonClick;
            portGroup.Controls.Add(connectButton);

            Label settings = AddLabel(portGroup,
                "115200 · 8N1 · DTR/RTS 关闭", 470, 36, 155);
            settings.ForeColor = Color.DimGray;

            tabs.Location = new Point(20, 168);
            tabs.Size = new Size(640, 260);
            Controls.Add(tabs);

            TabPage timePage = new TabPage("时间同步");
            TabPage alarmPage = new TabPage("闹钟设置");
            timePage.BackColor = SystemColors.Control;
            alarmPage.BackColor = SystemColors.Control;
            temperaturePage.BackColor = SystemColors.Control;
            screenPage.BackColor = SystemColors.Control;
            tabs.TabPages.Add(timePage);
            tabs.TabPages.Add(alarmPage);
            tabs.TabPages.Add(temperaturePage);
            tabs.TabPages.Add(screenPage);
            tabs.SelectedIndexChanged += delegate
            {
                nextTemperaturePollUtc = DateTime.MinValue;
            };

            GroupBox autoGroup = CreateGroup(timePage, "自动校准", 10, 8, 610, 92);
            pcClockLabel.Font = new Font("Consolas", 17F, FontStyle.Bold);
            pcClockLabel.AutoSize = true;
            pcClockLabel.Location = new Point(18, 37);
            autoGroup.Controls.Add(pcClockLabel);

            autoSyncButton.Text = "同步电脑当前时间";
            autoSyncButton.Location = new Point(408, 28);
            autoSyncButton.Size = new Size(180, 42);
            autoSyncButton.Click += AutoSyncButtonClick;
            autoGroup.Controls.Add(autoSyncButton);

            GroupBox manualGroup = CreateGroup(timePage, "手动设置", 10, 108, 610, 110);
            ConfigureTimeInput(hourInput, 0, 23, 22);
            ConfigureTimeInput(minuteInput, 0, 59, 116);
            ConfigureTimeInput(secondInput, 0, 59, 210);
            manualGroup.Controls.Add(hourInput);
            manualGroup.Controls.Add(minuteInput);
            manualGroup.Controls.Add(secondInput);
            AddLabel(manualGroup, "小时", 37, 24, 50);
            AddLabel(manualGroup, "分钟", 131, 24, 50);
            AddLabel(manualGroup, "秒", 231, 24, 35);
            AddLabel(manualGroup, ":", 97, 57, 15).Font =
                new Font("Consolas", 15F, FontStyle.Bold);
            AddLabel(manualGroup, ":", 191, 57, 15).Font =
                new Font("Consolas", 15F, FontStyle.Bold);

            fillNowButton.Text = "填入当前时间";
            fillNowButton.Location = new Point(292, 47);
            fillNowButton.Size = new Size(125, 36);
            fillNowButton.Click += delegate { FillCurrentTime(); };
            manualGroup.Controls.Add(fillNowButton);

            manualSyncButton.Text = "发送手动时间";
            manualSyncButton.Location = new Point(430, 42);
            manualSyncButton.Size = new Size(158, 46);
            manualSyncButton.Click += ManualSyncButtonClick;
            manualGroup.Controls.Add(manualSyncButton);

            Label alarmHelp = AddLabel(alarmPage,
                "时间/音乐与开关分别写入；勾选状态不是设备读回值。",
                14, 12, 590);
            alarmHelp.ForeColor = Color.DimGray;

            GroupBox alarmGroup = CreateGroup(alarmPage, "修改闹钟", 10, 42, 610, 174);
            BuildAlarmRow(alarmGroup, 0, 30, 8, 0);
            BuildAlarmRow(alarmGroup, 1, 75, 12, 0);
            BuildAlarmRow(alarmGroup, 2, 120, 16, 0);

            Label temperatureHelp = AddLabel(temperaturePage,
                "读取单片机板载热敏电阻测得的室温（精确到 0.1°C）。",
                18, 18, 590);
            temperatureHelp.ForeColor = Color.DimGray;

            temperatureValueLabel.Text = "--.- °C";
            temperatureValueLabel.Font = new Font("Consolas", 42F, FontStyle.Bold);
            temperatureValueLabel.TextAlign = ContentAlignment.MiddleCenter;
            temperatureValueLabel.Location = new Point(30, 55);
            temperatureValueLabel.Size = new Size(550, 90);
            temperaturePage.Controls.Add(temperatureValueLabel);

            temperatureUpdatedLabel.Text = "等待连接后读取";
            temperatureUpdatedLabel.TextAlign = ContentAlignment.MiddleCenter;
            temperatureUpdatedLabel.Location = new Point(30, 145);
            temperatureUpdatedLabel.Size = new Size(550, 28);
            temperatureUpdatedLabel.ForeColor = Color.DimGray;
            temperaturePage.Controls.Add(temperatureUpdatedLabel);

            temperatureAutoCheck.Text = "每秒自动刷新";
            temperatureAutoCheck.Checked = true;
            temperatureAutoCheck.Location = new Point(140, 182);
            temperatureAutoCheck.Size = new Size(145, 30);
            temperatureAutoCheck.CheckedChanged += delegate
            {
                nextTemperaturePollUtc = DateTime.MinValue;
            };
            temperaturePage.Controls.Add(temperatureAutoCheck);

            temperatureRefreshButton.Text = "立即读取";
            temperatureRefreshButton.Location = new Point(322, 179);
            temperatureRefreshButton.Size = new Size(130, 36);
            temperatureRefreshButton.Click += async delegate
            {
                await ReadTemperatureAsync(true);
            };
            temperaturePage.Controls.Add(temperatureRefreshButton);

            Label screenHelp = AddLabel(screenPage,
                "连续 60 秒没有按键时关闭数码管；时钟、闹钟和串口继续运行。",
                18, 20, 590);
            screenHelp.ForeColor = Color.DimGray;

            screenAutoOffCheck.Text = "启用自动息屏";
            screenAutoOffCheck.Checked = true;
            screenAutoOffCheck.Location = new Point(88, 84);
            screenAutoOffCheck.Size = new Size(180, 35);
            screenPage.Controls.Add(screenAutoOffCheck);

            screenSaveButton.Text = "写入单片机";
            screenSaveButton.Location = new Point(340, 81);
            screenSaveButton.Size = new Size(170, 42);
            screenSaveButton.Click += ScreenSaveButtonClick;
            screenPage.Controls.Add(screenSaveButton);

            Label screenNote = AddLabel(screenPage,
                "按键先唤醒屏幕；闹钟响起也会亮屏。开关状态断电保存。",
                18, 145, 590);
            screenNote.ForeColor = Color.DimGray;
            Label screenPending = AddLabel(screenPage,
                "勾选框是待写入值，不代表从单片机读回的当前状态。",
                18, 180, 590);
            screenPending.ForeColor = Color.DimGray;

            statusLabel.Text = "尚未连接";
            statusLabel.AutoSize = false;
            statusLabel.Location = new Point(24, 438);
            statusLabel.Size = new Size(630, 25);
            statusLabel.ForeColor = Color.FromArgb(45, 90, 135);
            Controls.Add(statusLabel);

            logBox.Location = new Point(20, 468);
            logBox.Size = new Size(640, 72);
            logBox.Multiline = true;
            logBox.ReadOnly = true;
            logBox.ScrollBars = ScrollBars.Vertical;
            logBox.BackColor = Color.White;
            logBox.Font = new Font("Consolas", 9.5F);
            Controls.Add(logBox);

            UpdateButtonState();
        }

        private GroupBox CreateGroup(Control parent, string text, int x, int y,
            int width, int height)
        {
            GroupBox group = new GroupBox();
            group.Text = text;
            group.Location = new Point(x, y);
            group.Size = new Size(width, height);
            parent.Controls.Add(group);
            return group;
        }

        private void BuildAlarmRow(Control parent, int index, int y,
            int defaultHour, int defaultMinute)
        {
            AddLabel(parent, "闹钟 " + (index + 1), 18, y + 3, 62);

            NumericUpDown hour = alarmHourInputs[index];
            hour.Minimum = 0;
            hour.Maximum = 23;
            hour.Value = defaultHour;
            hour.Location = new Point(82, y);
            hour.Size = new Size(55, 30);
            hour.TextAlign = HorizontalAlignment.Center;
            parent.Controls.Add(hour);
            AddLabel(parent, "时", 140, y + 3, 25);

            NumericUpDown minute = alarmMinuteInputs[index];
            minute.Minimum = 0;
            minute.Maximum = 59;
            minute.Value = defaultMinute;
            minute.Location = new Point(165, y);
            minute.Size = new Size(55, 30);
            minute.TextAlign = HorizontalAlignment.Center;
            parent.Controls.Add(minute);
            AddLabel(parent, "分", 223, y + 3, 25);

            ComboBox music = alarmMusicCombos[index];
            music.DropDownStyle = ComboBoxStyle.DropDownList;
            music.Items.AddRange(MelodyNames);
            music.SelectedIndex = index;
            music.Location = new Point(248, y);
            music.Size = new Size(170, 30);
            music.DropDownWidth = 260;
            parent.Controls.Add(music);

            CheckBox enabled = alarmEnableChecks[index];
            enabled.Text = "启用";
            enabled.Checked = true;
            enabled.Location = new Point(425, y + 2);
            enabled.Size = new Size(58, 28);
            parent.Controls.Add(enabled);

            Button button = alarmSyncButtons[index];
            button.Text = "写时间";
            button.Tag = index;
            button.Location = new Point(484, y - 2);
            button.Size = new Size(58, 34);
            button.Click += AlarmSyncButtonClick;
            parent.Controls.Add(button);

            Button switchButton = alarmSwitchButtons[index];
            switchButton.Text = "写开关";
            switchButton.Tag = index;
            switchButton.Location = new Point(546, y - 2);
            switchButton.Size = new Size(58, 34);
            switchButton.Click += AlarmSwitchButtonClick;
            parent.Controls.Add(switchButton);
        }

        private static Label AddLabel(Control parent, string text, int x, int y,
            int width)
        {
            Label label = new Label();
            label.Text = text;
            label.AutoSize = false;
            label.Location = new Point(x, y);
            label.Size = new Size(width, 28);
            parent.Controls.Add(label);
            return label;
        }

        private static void ConfigureTimeInput(NumericUpDown input, int minimum,
            int maximum, int x)
        {
            input.Minimum = minimum;
            input.Maximum = maximum;
            input.Location = new Point(x, 52);
            input.Size = new Size(65, 30);
            input.TextAlign = HorizontalAlignment.Center;
        }

        private void RefreshPorts()
        {
            string previous = portCombo.Text;
            string[] ports = SerialPort.GetPortNames().OrderBy(p => p).ToArray();
            portCombo.Items.Clear();
            portCombo.Items.AddRange(ports);

            if (ports.Contains(previous))
                portCombo.SelectedItem = previous;
            else if (ports.Length > 0)
                portCombo.SelectedIndex = 0;

            if (!IsConnected)
                statusLabel.Text = ports.Length == 0
                    ? "未发现串口，请检查 USB 转串口模块"
                    : "发现 " + ports.Length + " 个串口，请选择后连接";
        }

        private async void ConnectButtonClick(object sender, EventArgs e)
        {
            if (IsConnected)
            {
                Disconnect();
                return;
            }

            if (String.IsNullOrWhiteSpace(portCombo.Text))
            {
                MessageBox.Show("请先选择串口。", "未选择串口",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            SetBusy(true);
            statusLabel.Text = "正在连接 " + portCombo.Text + "…";
            try
            {
                SerialPort candidate = new SerialPort(portCombo.Text, BaudRate,
                    Parity.None, 8, StopBits.One);
                candidate.Handshake = Handshake.None;
                candidate.DtrEnable = false;
                candidate.RtsEnable = false;
                candidate.ReadTimeout = 1800;
                candidate.WriteTimeout = 1000;
                candidate.Open();
                serialPort = candidate;

                await Task.Delay(1500);
                if (!IsConnected) return;

                serialPort.DiscardInBuffer();
                serialPort.DiscardOutBuffer();
                statusLabel.Text = "已连接 " + serialPort.PortName +
                    "，可以同步时间、设置闹钟或读取温度";
                nextTemperaturePollUtc = DateTime.MinValue;
                AppendLog("串口已连接，DTR/RTS 已关闭，启动等待完成。",
                    Color.DarkGreen);
            }
            catch (Exception ex)
            {
                Disconnect();
                statusLabel.Text = "连接失败：" + ex.Message;
                MessageBox.Show("无法打开串口：\r\n\r\n" + ex.Message +
                    "\r\n\r\n请关闭烧录软件或其他串口助手后重试。",
                    "连接失败", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                SetBusy(false);
            }
        }

        private async void AutoSyncButtonClick(object sender, EventArgs e)
        {
            DateTime now = DateTime.Now;
            await SendTimeAsync(now.Hour, now.Minute, now.Second, "自动校准");
        }

        private async void ManualSyncButtonClick(object sender, EventArgs e)
        {
            await SendTimeAsync((int)hourInput.Value, (int)minuteInput.Value,
                (int)secondInput.Value, "手动设置");
        }

        private async void AlarmSyncButtonClick(object sender, EventArgs e)
        {
            Button button = (Button)sender;
            int index = (int)button.Tag;
            int hour = (int)alarmHourInputs[index].Value;
            int minute = (int)alarmMinuteInputs[index].Value;
            int melody = alarmMusicCombos[index].SelectedIndex;
            byte[] frame = BuildAlarmFrame(index, hour, minute, melody);
            string value = String.Format("闹钟 {0} = {1:00}:{2:00}，{3}",
                index + 1, hour, minute, MelodyNames[melody]);
            await SendFrameAsync(frame, "修改闹钟", value);
        }

        private async void AlarmSwitchButtonClick(object sender, EventArgs e)
        {
            Button button = (Button)sender;
            int index = (int)button.Tag;
            bool enabled = alarmEnableChecks[index].Checked;
            byte[] frame = BuildAlarmSwitchFrame(index, enabled);
            string value = String.Format("闹钟 {0} {1}", index + 1,
                enabled ? "启用" : "关闭");
            await SendFrameAsync(frame, "设置闹钟开关", value);
        }

        private async void ScreenSaveButtonClick(object sender, EventArgs e)
        {
            bool enabled = screenAutoOffCheck.Checked;
            await SendFrameAsync(BuildScreenFrame(enabled), "设置自动息屏",
                enabled ? "启用（60 秒无按键后息屏）" : "关闭");
        }

        private async void ClockTimerTick(object sender, EventArgs e)
        {
            UpdatePcClock();
            if (tabs.SelectedTab == temperaturePage && IsConnected && !busy &&
                temperatureAutoCheck.Checked &&
                DateTime.UtcNow >= nextTemperaturePollUtc)
            {
                nextTemperaturePollUtc = DateTime.UtcNow.AddSeconds(1);
                await ReadTemperatureAsync(false);
            }
        }

        private async Task ReadTemperatureAsync(bool manual)
        {
            if (!IsConnected)
            {
                if (manual)
                    MessageBox.Show("请先连接串口。", "尚未连接",
                        MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }
            if (busy) return;

            SetBusy(true);
            temperatureUpdatedLabel.Text = "正在读取…";
            byte[] response = new byte[4];
            int receivedBytes = 0;
            try
            {
                byte[] reply = await Task.Run(delegate
                {
                    lock (serialLock)
                    {
                        byte[] request = BuildTemperatureRequest();
                        serialPort.DiscardInBuffer();
                        serialPort.Write(request, 0, request.Length);
                        Stopwatch stopwatch = Stopwatch.StartNew();
                        for (int i = 0; i < response.Length; i++)
                        {
                            int remaining = 1800 - (int)stopwatch.ElapsedMilliseconds;
                            if (remaining <= 0)
                                throw new TimeoutException("温度查询超时");
                            serialPort.ReadTimeout = remaining;
                            response[i] = (byte)serialPort.ReadByte();
                            receivedBytes++;
                        }
                        return response;
                    }
                });

                if (reply[0] == 0x05 && reply[1] == 0xFF &&
                    reply[2] == 0xFF && reply[3] == FrameTail)
                    throw new FormatException("单片机 ADC 采样超时");
                if (reply[0] != 0x05 || reply[1] > 99 ||
                    reply[2] > 9 || reply[3] != FrameTail)
                    throw new FormatException("单片机返回了无效温度帧");

                temperatureValueLabel.Text = String.Format("{0}.{1} °C",
                    reply[1], reply[2]);
                temperatureUpdatedLabel.Text = "最近读取：" +
                    DateTime.Now.ToString("HH:mm:ss");
                statusLabel.Text = "温度读取成功：" + temperatureValueLabel.Text;
                if (manual)
                    AppendLog(DateTime.Now.ToString("HH:mm:ss") +
                        "  TX  05 AA  RX  " +
                        BitConverter.ToString(reply).Replace('-', ' '),
                        Color.DarkGreen);
            }
            catch (TimeoutException ex)
            {
                temperatureAutoCheck.Checked = false;
                string received = receivedBytes == 0 ? "无返回字节" :
                    BitConverter.ToString(response, 0, receivedBytes).Replace('-', ' ');
                temperatureUpdatedLabel.Text = receivedBytes == 0 ?
                    "单片机未回复；请确认已烧录支持温度查询的 HEX" :
                    "温度回包不完整（已收到 " + receivedBytes + " 字节）";
                statusLabel.Text = "温度读取失败：" + temperatureUpdatedLabel.Text;
                AppendLog(DateTime.Now.ToString("HH:mm:ss") +
                    "  TX  05 AA  RX  " + received + "  " + ex.Message,
                    Color.DarkRed);
                if (manual)
                    MessageBox.Show(temperatureUpdatedLabel.Text + "。",
                        "读取失败", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            catch (FormatException ex)
            {
                temperatureAutoCheck.Checked = false;
                temperatureUpdatedLabel.Text = ex.Message;
                statusLabel.Text = "温度读取失败：" + ex.Message;
                AppendLog(DateTime.Now.ToString("HH:mm:ss") +
                    "  TX  05 AA  RX  " +
                    BitConverter.ToString(response).Replace('-', ' ') +
                    "  " + ex.Message,
                    Color.DarkRed);
            }
            catch (Exception ex)
            {
                Disconnect();
                temperatureUpdatedLabel.Text = "串口连接已断开";
                statusLabel.Text = "温度读取失败：" + ex.Message;
                AppendLog(DateTime.Now.ToString("HH:mm:ss") + "  ERROR  " +
                    ex.Message, Color.DarkRed);
            }
            finally
            {
                nextTemperaturePollUtc = DateTime.UtcNow.AddSeconds(1);
                SetBusy(false);
            }
        }

        private async Task SendTimeAsync(int hour, int minute, int second,
            string mode)
        {
            byte[] frame = BuildTimeFrame(hour, minute, second);
            string value = String.Format("{0:00}:{1:00}:{2:00}",
                hour, minute, second);
            await SendFrameAsync(frame, mode, value);
        }

        private async Task SendFrameAsync(byte[] frame, string mode,
            string value)
        {
            if (!IsConnected)
            {
                MessageBox.Show("请先连接串口。", "尚未连接",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            SetBusy(true);
            statusLabel.Text = mode + "发送中，等待单片机 ACK…";

            try
            {
                await Task.Run(delegate
                {
                    lock (serialLock)
                    {
                        serialPort.DiscardInBuffer();
                        serialPort.Write(frame, 0, frame.Length);

                        Stopwatch stopwatch = Stopwatch.StartNew();
                        while (stopwatch.ElapsedMilliseconds < 1800)
                        {
                            int remaining = 1800 -
                                (int)stopwatch.ElapsedMilliseconds;
                            serialPort.ReadTimeout = Math.Max(100, remaining);
                            int reply = serialPort.ReadByte();
                            if (reply == Ack) return;
                        }
                    }
                    throw new TimeoutException("未收到单片机的 0x06 应答");
                });

                string frameText = BitConverter.ToString(frame).Replace('-', ' ');
                statusLabel.Text = mode + "成功：" + value + "，已收到 ACK 06";
                AppendLog(DateTime.Now.ToString("HH:mm:ss") + "  TX  " +
                    frameText + "  RX  06", Color.DarkGreen);
            }
            catch (TimeoutException ex)
            {
                statusLabel.Text = "发送完成，但单片机没有确认";
                AppendLog(DateTime.Now.ToString("HH:mm:ss") + "  " + ex.Message,
                    Color.DarkRed);
                MessageBox.Show("数据已经发出，但没有收到单片机 ACK。\r\n\r\n" +
                    "请确认已烧录带 ACK 的最新 HEX，并检查 TX/RX 是否交叉连接。",
                    "同步未确认", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            catch (Exception ex)
            {
                AppendLog(DateTime.Now.ToString("HH:mm:ss") + "  ERROR  " +
                    ex.Message, Color.DarkRed);
                Disconnect();
                statusLabel.Text = "通信失败：" + ex.Message;
                MessageBox.Show("串口通信失败：\r\n\r\n" + ex.Message,
                    "通信失败", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                SetBusy(false);
            }
        }

        private static byte[] BuildTimeFrame(int hour, int minute, int second)
        {
            return new byte[]
            {
                FrameHead, ToBcd(hour), ToBcd(minute), ToBcd(second), FrameTail
            };
        }

        private static byte[] BuildAlarmFrame(int index, int hour, int minute,
            int melody)
        {
            return new byte[]
            {
                0x03, (byte)(index + 1), ToBcd(hour), ToBcd(minute),
                (byte)(melody + 1), FrameTail
            };
        }

        private static byte[] BuildAlarmSwitchFrame(int index, bool enabled)
        {
            return new byte[]
            {
                0x04, (byte)(index + 1), enabled ? (byte)1 : (byte)0,
                FrameTail
            };
        }

        private static byte[] BuildTemperatureRequest()
        {
            return new byte[] { 0x05, FrameTail };
        }

        private static byte[] BuildScreenFrame(bool enabled)
        {
            return new byte[] { 0x06, enabled ? (byte)1 : (byte)0, FrameTail };
        }

        private static byte ToBcd(int value)
        {
            return (byte)(((value / 10) << 4) | (value % 10));
        }

        private void FillCurrentTime()
        {
            DateTime now = DateTime.Now;
            hourInput.Value = now.Hour;
            minuteInput.Value = now.Minute;
            secondInput.Value = now.Second;
        }

        private void UpdatePcClock()
        {
            pcClockLabel.Text = DateTime.Now.ToString("yyyy-MM-dd  HH:mm:ss");
        }

        private bool IsConnected
        {
            get { return serialPort != null && serialPort.IsOpen; }
        }

        private void SetBusy(bool value)
        {
            busy = value;
            UpdateButtonState();
        }

        private void UpdateButtonState()
        {
            bool connected = IsConnected;
            portCombo.Enabled = !connected && !busy;
            refreshButton.Enabled = !connected && !busy;
            connectButton.Enabled = !busy;
            connectButton.Text = connected ? "断开" : "连接";
            autoSyncButton.Enabled = connected && !busy;
            manualSyncButton.Enabled = connected && !busy;
            foreach (Button button in alarmSyncButtons)
                button.Enabled = connected && !busy;
            foreach (Button button in alarmSwitchButtons)
                button.Enabled = connected && !busy;
            temperatureRefreshButton.Enabled = connected && !busy;
            screenSaveButton.Enabled = connected && !busy;
        }

        private void Disconnect()
        {
            SerialPort oldPort = serialPort;
            serialPort = null;
            if (oldPort != null)
            {
                try
                {
                    oldPort.DtrEnable = false;
                    oldPort.RtsEnable = false;
                    if (oldPort.IsOpen) oldPort.Close();
                }
                catch
                {
                }
                oldPort.Dispose();
            }
            statusLabel.Text = "串口已断开";
            temperatureValueLabel.Text = "--.- °C";
            temperatureUpdatedLabel.Text = "等待连接后读取";
            UpdateButtonState();
        }

        private void AppendLog(string text, Color color)
        {
            logBox.ForeColor = color;
            logBox.AppendText(text + Environment.NewLine);
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            clockTimer.Stop();
            Disconnect();
            base.OnFormClosing(e);
        }
    }
}
