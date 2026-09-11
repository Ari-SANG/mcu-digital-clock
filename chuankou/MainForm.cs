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
        private readonly Timer clockTimer = new Timer();
        private readonly object serialLock = new object();

        private SerialPort serialPort;
        private bool busy;

        public MainForm()
        {
            Text = "单片机串口时间同步";
            ClientSize = new Size(680, 520);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            Font = new Font("Microsoft YaHei UI", 10F);

            BuildInterface();
            RefreshPorts();
            FillCurrentTime();
            UpdatePcClock();

            clockTimer.Interval = 200;
            clockTimer.Tick += delegate { UpdatePcClock(); };
            clockTimer.Start();
        }

        private void BuildInterface()
        {
            Label title = new Label();
            title.Text = "串口时间同步工具";
            title.Font = new Font("Microsoft YaHei UI", 18F, FontStyle.Bold);
            title.AutoSize = true;
            title.Location = new Point(22, 18);
            Controls.Add(title);

            GroupBox portGroup = CreateGroup("串口连接", 20, 62, 640, 94);
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

            GroupBox autoGroup = CreateGroup("自动校准", 20, 168, 640, 100);
            pcClockLabel.Font = new Font("Consolas", 17F, FontStyle.Bold);
            pcClockLabel.AutoSize = true;
            pcClockLabel.Location = new Point(18, 37);
            autoGroup.Controls.Add(pcClockLabel);

            autoSyncButton.Text = "同步电脑当前时间";
            autoSyncButton.Location = new Point(438, 32);
            autoSyncButton.Size = new Size(180, 42);
            autoSyncButton.Click += AutoSyncButtonClick;
            autoGroup.Controls.Add(autoSyncButton);

            GroupBox manualGroup = CreateGroup("手动设置", 20, 280, 640, 112);
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
            fillNowButton.Location = new Point(316, 47);
            fillNowButton.Size = new Size(130, 36);
            fillNowButton.Click += delegate { FillCurrentTime(); };
            manualGroup.Controls.Add(fillNowButton);

            manualSyncButton.Text = "发送手动时间";
            manualSyncButton.Location = new Point(458, 42);
            manualSyncButton.Size = new Size(160, 46);
            manualSyncButton.Click += ManualSyncButtonClick;
            manualGroup.Controls.Add(manualSyncButton);

            statusLabel.Text = "尚未连接";
            statusLabel.AutoSize = false;
            statusLabel.Location = new Point(24, 408);
            statusLabel.Size = new Size(630, 25);
            statusLabel.ForeColor = Color.FromArgb(45, 90, 135);
            Controls.Add(statusLabel);

            logBox.Location = new Point(20, 438);
            logBox.Size = new Size(640, 62);
            logBox.Multiline = true;
            logBox.ReadOnly = true;
            logBox.ScrollBars = ScrollBars.Vertical;
            logBox.BackColor = Color.White;
            logBox.Font = new Font("Consolas", 9.5F);
            Controls.Add(logBox);

            UpdateButtonState();
        }

        private GroupBox CreateGroup(string text, int x, int y, int width, int height)
        {
            GroupBox group = new GroupBox();
            group.Text = text;
            group.Location = new Point(x, y);
            group.Size = new Size(width, height);
            Controls.Add(group);
            return group;
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
                    "，可以同步时间";
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

        private async Task SendTimeAsync(int hour, int minute, int second,
            string mode)
        {
            if (!IsConnected)
            {
                MessageBox.Show("请先连接串口。", "尚未连接",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            byte[] frame = BuildFrame(hour, minute, second);
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
                statusLabel.Text = mode + "成功：" +
                    String.Format("{0:00}:{1:00}:{2:00}", hour, minute, second) +
                    "，已收到 ACK 06";
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

        private static byte[] BuildFrame(int hour, int minute, int second)
        {
            return new byte[]
            {
                FrameHead, ToBcd(hour), ToBcd(minute), ToBcd(second), FrameTail
            };
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
