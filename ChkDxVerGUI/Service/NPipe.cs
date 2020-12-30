using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO.Pipes;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace ChkDxVerGUI.Service
{
    public class NPipe : IDisposable
    {
        private readonly string _name;
        private readonly NamedPipeServerStream server;

        private BackgroundWorker worker;
        public event Action<string> OnData;
        public NPipe(string name)
        {
            _name = name;
            server = new NamedPipeServerStream(name, PipeDirection.In, 1);
        }

        public async Task RunAsync()
        {
            await server.WaitForConnectionAsync();

            worker = new BackgroundWorker();
            worker.DoWork += WorkerOnDoWork;
            worker.RunWorkerAsync();
        }

        private void WorkerOnDoWork(object sender, DoWorkEventArgs e)
        {
            while (!e.Cancel)
            {
                if (server.IsConnected)
                {
                    if (server.CanRead)
                    {
                        var buffer = new byte[256];
                        int readed = server.Read(buffer, 0, buffer.Length);
                        if (readed > 0)
                            OnData?.Invoke(Encoding.Default.GetString(buffer));
                    }
                }
                else
                {
                    try
                    {
                        server?.Disconnect();
                        server.WaitForConnection();
                        Console.WriteLine("已连接");
                    }
                    catch (Exception)
                    {
                        // ignore
                    }
                }
            }

            Debug.Print("Work exit.");
        }


        public void Dispose()
        {
            worker?.Dispose();
            server?.Dispose();
        }
    }
}