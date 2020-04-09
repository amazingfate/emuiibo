using System;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Net;
using System.Text;
using FluentFTP;

namespace emutool
{
    public partial class MainForm : Form
    {
        public static AmiiboAPI.AmiiboList Amiibos = null;

        public static List<string> AmiiboSeries = null;
        public static List<AmiiboAPI.Amiibo> CurrentSeriesAmiibos = null;


        public static bool HasAmiibos()
        {
            if (Amiibos != null)
            {
                return Amiibos.GetAmiiboCount() > 0;
            }
            return false;
        }

        public MainForm()
        {
            InitializeComponent();
            Amiibos = AmiiboAPI.GetAllAmiibos();

            if (HasAmiibos())
            {
                toolStripStatusLabel1.Text = "AmiiboAPI was accessed - amiibo list was loaded.";
                AmiiboSeries = Amiibos.GetAmiiboSeries();

                if (AmiiboSeries.Any())
                {
                    foreach (var series in AmiiboSeries)
                    {
                        comboBox1.Items.Add(series);
                    }
                    comboBox1.SelectedIndex = 0;
                }
            }
            else
            {
                toolStripStatusLabel1.Text = "Unable to download amiibo list from AmiiboAPI.";
                toolStripStatusLabel1.Image = Properties.Resources.ErrorIcon;
                groupBox1.Enabled = false;
                groupBox2.Enabled = false;
                groupBox3.Enabled = false;
            }
        }

        private void ComboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            comboBox2.Items.Clear();

            if (HasAmiibos())
            {
                var series = comboBox1.Text;
                CurrentSeriesAmiibos = Amiibos.GetAmiibosBySeries(series);
                if (CurrentSeriesAmiibos.Any())
                {
                    foreach (var amiibo in CurrentSeriesAmiibos)
                    {
                        comboBox2.Items.Add(amiibo.AmiiboName);
                    }
                }
                comboBox2.SelectedIndex = 0;
            }
        }

        private void comboBox2_SelectedIndexChanged(object sender, EventArgs e)
        {
            try
            {
                var cur_amiibo = CurrentSeriesAmiibos[comboBox2.SelectedIndex];
                pictureBox1.ImageLocation = cur_amiibo.ImageURL;
                textBox1.Text = cur_amiibo.AmiiboName;
            }
            catch (Exception ex)
            {
                ExceptionUtils.LogExceptionMessage(ex);
            }
        }

        private void Button1_Click(object sender, EventArgs e)
        {
            try
            {
                var cur_amiibo = CurrentSeriesAmiibos[comboBox2.SelectedIndex];
                if (string.IsNullOrEmpty(textBox1.Text))
                {
                    MessageBox.Show("No amiibo name was specified.", Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                bool use_name_as_dir = checkBox3.Checked;
                if (!use_name_as_dir && string.IsNullOrEmpty(textBox2.Text))
                {
                    MessageBox.Show("No amiibo directory name was specified.", Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                string name = textBox1.Text;
                string dir_name = name;
                if (!use_name_as_dir)
                {
                    dir_name = textBox2.Text;
                }

                if (!chkFTP.Checked)
                {
                    // save to local location
                    string emuiibo_dir = "";
                    if (DriveInfo.GetDrives().Any())
                    {
                        foreach (var drive in DriveInfo.GetDrives())
                        {
                            if (drive.IsReady)
                            {
                                if (Directory.Exists(Path.Combine(drive.Name, Path.Combine("emuiibo", "amiibo"))))
                                {
                                    emuiibo_dir = Path.Combine(drive.Name, Path.Combine("emuiibo", "amiibo"));
                                }
                                else if (Directory.Exists(Path.Combine(drive.Name, "emuiibo")))
                                {
                                    Directory.CreateDirectory(Path.Combine(drive.Name, Path.Combine("emuiibo", "amiibo")));
                                    emuiibo_dir = Path.Combine(drive.Name, Path.Combine("emuiibo", "amiibo"));
                                }
                                if (!string.IsNullOrEmpty(emuiibo_dir))
                                {
                                    MessageBox.Show($"Emuiibo directory was found in drive '{drive.VolumeLabel}', so defaulting to that directory.", Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
                                }
                            }
                        }
                    }
                    FolderBrowserDialog folderBrowserDialog = new FolderBrowserDialog
                    {
                        Description = "Select root directory to generate the virtual amiibo on",
                        ShowNewFolderButton = false,
                        SelectedPath = emuiibo_dir
                    };
                    if (folderBrowserDialog.ShowDialog() == DialogResult.OK)
                    {
                        string dir = Path.Combine(folderBrowserDialog.SelectedPath, dir_name);

                        if (MessageBox.Show($"Virtual amiibo will be created in '{dir}'.\n\nThe directory will be deleted if it already exists.\n\nProceed with amiibo creation?", Text, MessageBoxButtons.OKCancel, MessageBoxIcon.Question) != DialogResult.OK)
                        {
                            return;
                        }

                        var amiibo = AmiiboUtils.BuildAmiibo(cur_amiibo, name);
                        amiibo.Save(dir, checkBox1.Checked, checkBox2.Checked);

                        MessageBox.Show("Virtual amiibo was successfully created.", Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
                else
                {
                    // save to FTP location
                    try
                    {
                        var tmp_path = Environment.CurrentDirectory + "\\tmp";
                        var dir = Path.Combine(tmp_path, dir_name);
                        var sd_folder = $"/emuiibo/amiibo/{dir_name}/";

                        // first let's validate the FTP address
                        IPAddress ip;
                        if (!IPAddress.TryParse(txtFTPAddress.Text, out ip))
                        {
                            MessageBox.Show("FTP address is invalid", Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
                            return;
                        }

                        int port;
                        if (!Int32.TryParse(txtFTPPort.Text, out port))
                        {
                            MessageBox.Show("FTP Port is invalid", Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
                            return;
                        }

                        if (MessageBox.Show($"Virtual amiibo will be created in 'ftp://{ip.ToString()}:{port}{sd_folder}'.\n\nThe directory will be deleted if it already exists.\n\nProceed with amiibo creation?", Text, MessageBoxButtons.OKCancel, MessageBoxIcon.Question) != DialogResult.OK)
                        {
                            return;
                        }

                        // create a tmp directory to output the resulting files from amiibo.Save() before transfer
                        if (!Directory.Exists(tmp_path))
                        {
                            Directory.CreateDirectory(tmp_path);
                        }

                        var amiibo = AmiiboUtils.BuildAmiibo(cur_amiibo, name);
                        amiibo.Save(dir, checkBox1.Checked, checkBox2.Checked);

                        var success = true;
                        using (FtpClient client = new FtpClient(ip.ToString(), port, new NetworkCredential("", "")))
                        {
                            client.ConnectTimeout = 1000;
                            client.Connect();


                            foreach (var file in Directory.GetFiles(dir))
                            {
                                string file_name = Path.GetFileName(file);
                                // upload each file created, creating directories along the way
                                var status = client.UploadFile(file, sd_folder + file_name, createRemoteDir: true);
                                success &= status == FtpStatus.Success;
                            }
                            client.Disconnect();
                        }

                        if (success)
                        {
                            // recursively delete the tmp directory to clean up
                            Directory.Delete(tmp_path, true);
                            MessageBox.Show("Virtual amiibo was successfully created.", Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
                        }
                        else
                        {
                            throw new Exception("Error during FTP upload, please try again");
                        }
                    }
                    catch (Exception ftp_ex)
                    {
                        ExceptionUtils.LogExceptionMessage(ftp_ex);
                    }
                }
            }
            catch (Exception ex)
            {
                ExceptionUtils.LogExceptionMessage(ex);
            }
        }

        private void CheckBox1_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox1.Checked)
            {
                if (MessageBox.Show("Please, keep in mind that the random UUID feature might cause in some cases (Smash Bros., for example) the amiibo not to be recognized.\n(for example, when saving data to the amiibo, it could not be recognized as the original one)\n\nWould you really like to enable this feature?", "emutool - Randomize UUID", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes)
                {
                    checkBox1.Checked = false;
                }
            }
        }

        private void CheckBox3_CheckedChanged(object sender, EventArgs e)
        {
            textBox2.Enabled = !checkBox3.Checked;
        }

        private void chkFTP_CheckedChanged(object sender, EventArgs e)
        {
            txtFTPAddress.Enabled = chkFTP.Checked;
            txtFTPPort.Enabled = chkFTP.Checked;
        }
    }
}