using System.ComponentModel;
using System.Windows;
using ChkDxVerGUI.Model;
using ChkDxVerGUI.Service;
using ChkDxVerGUI.ViewModel;

namespace ChkDxVerGUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private NPipe pipe;
        public MainWindow()
        {
            InitializeComponent();
            DataContext = new MainWindowViewModel();
            Loaded += OnLoaded;
            Closing += OnClosing;
        }

        private async void OnLoaded(object sender, RoutedEventArgs e)
        {
            pipe = new NPipe("ChkDxVer");
            pipe.OnData += PipeOnOnData;
            await pipe.RunAsync();
        }

        private void PipeOnOnData(string str)
        {
            Dispatcher.Invoke(() =>
            {
                if (DataContext is MainWindowViewModel vm)
                {
                    vm.ChkDxs.Add(new ChkDxModel()
                    {
                        ProcessName = System.IO.Path.GetFileName(vm.ProcessPath),
                        DxVer = str
                    });

                    vm.Spwan.CloseMainWindow();
                    vm.Spwan.Dispose();
                }
            });
        }

        private void OnClosing(object sender, CancelEventArgs e)
        {
            pipe.Dispose();
        }
    }
}
