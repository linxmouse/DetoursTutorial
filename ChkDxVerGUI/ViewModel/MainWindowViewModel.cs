using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Ink;
using ChkDxVerGUI.Model;
using Microsoft.Toolkit.Mvvm.ComponentModel;
using Microsoft.Toolkit.Mvvm.Input;
using Microsoft.Win32;

namespace ChkDxVerGUI.ViewModel
{
    public class MainWindowViewModel: ObservableObject
    {
        public Process Spwan { get; set; }

        private ObservableCollection<ChkDxModel> _chkDxs = new ObservableCollection<ChkDxModel>();
        public ObservableCollection<ChkDxModel> ChkDxs
        {
            get => _chkDxs;
            set => SetProperty(ref _chkDxs, value);
        }

        private string _processPath;
        public string ProcessPath
        {
            get => _processPath;
            set => SetProperty(ref _processPath, value, nameof(ProcessPath));
        }

        public RelayCommand ViewCommand => new Lazy<RelayCommand>(
                new RelayCommand(() =>
                {
                    OpenFileDialog ofd = new OpenFileDialog();
                    var result = ofd.ShowDialog();
                    if (result.HasValue && result.Value)
                    {
                        ProcessPath = ofd.FileName;
                    }
                }))
            .Value;

        public RelayCommand ChkCommand => new Lazy<RelayCommand>(
                new RelayCommand(() =>
                {
                    ProcessStartInfo startinfo = new ProcessStartInfo();
                    //startinfo.UseShellExecute = true;
                    startinfo.FileName = Path.Combine(Directory.GetCurrentDirectory(), "DetoursInjector.exe");
                    var dllPath = Path.Combine(Directory.GetCurrentDirectory(), "ChkDxVer32.dll");
                    startinfo.Arguments = "\""+ ProcessPath + "\"" + " " + "\"" + dllPath + "\"";
                    startinfo.WorkingDirectory = Directory.GetCurrentDirectory();
                    startinfo.CreateNoWindow = true;
                    startinfo.WindowStyle = ProcessWindowStyle.Hidden;
                    Spwan = Process.Start(startinfo);
                }/*, () => !string.IsNullOrWhiteSpace(ProcessPath)*/))
            .Value;

        public RelayCommand ExportCommand => new Lazy<RelayCommand>(
                new RelayCommand(() =>
                {
                    MessageBoxResult result = MessageBox.Show("确定要导出吗？",
                        "提示", MessageBoxButton.OKCancel);
                    if (result == MessageBoxResult.OK)
                    {
                        string str = JsonSerializer.Serialize(ChkDxs, new JsonSerializerOptions()
                        {
                            WriteIndented = true
                        });
                        string path = DateTime.Now.ToLongDateString() + ".json";
                        File.WriteAllText(path, str);
                    }
                }))
            .Value;
    }
}