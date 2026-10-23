using System.IO.Compression;
using System.Reflection;
using System.Text.Json;
using Microsoft.Win32;

namespace GridlockSetup;

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        ApplicationConfiguration.Initialize();
        Application.Run(new SetupForm());
    }
}

internal sealed class SetupForm : Form
{
    private readonly Label engineLabel = new();
    private readonly TextBox folderBox = new();
    private readonly Button installButton = new();
    private readonly Button launchButton = new();
    private string? detectedEditor;
    private string? installedProject;

    public SetupForm()
    {
        Text = "Gridlock — Unreal Workspace Setup";
        StartPosition = FormStartPosition.CenterScreen;
        MinimumSize = new Size(650, 340);
        Size = new Size(760, 410);
        Font = new Font("Segoe UI", 10F);
        BackColor = Color.FromArgb(19, 26, 35);
        ForeColor = Color.FromArgb(239, 244, 250);

        var title = new Label { Text = "GRIDLOCK", Font = new Font("Segoe UI", 25F, FontStyle.Bold), AutoSize = true, Location = new Point(28, 22), ForeColor = Color.FromArgb(87, 205, 221) };
        var subtitle = new Label { Text = "Unreal Engine source workspace setup", AutoSize = true, Location = new Point(32, 70), ForeColor = Color.Gainsboro };
        engineLabel.SetBounds(32, 116, 680, 42);
        engineLabel.Text = "Checking for Unreal Engine 5.4…";
        var folderLabel = new Label { Text = "New workspace folder", AutoSize = true, Location = new Point(32, 180) };
        folderBox.SetBounds(32, 208, 560, 31);
        folderBox.Text = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "Unreal Projects", "Gridlock");
        var browse = new Button { Text = "Browse…", Bounds = new Rectangle(602, 207, 105, 32) };
        browse.Click += (_, _) => BrowseFolder();
        installButton.Text = "Create workspace";
        installButton.Bounds = new Rectangle(32, 270, 190, 42);
        installButton.Click += async (_, _) => await InstallWorkspaceAsync();
        launchButton.Text = "Open in Unreal Editor";
        launchButton.Bounds = new Rectangle(238, 270, 190, 42);
        launchButton.Enabled = false;
        launchButton.Click += (_, _) => LaunchEditor();
        var locate = new Button { Text = "Locate UE 5.4…", Bounds = new Rectangle(448, 270, 190, 42) };
        locate.Click += (_, _) => LocateEditor();
        Controls.AddRange([title, subtitle, engineLabel, folderLabel, folderBox, browse, installButton, launchButton, locate]);
        DetectEngine();
        FindExistingWorkspace();
    }

    private void BrowseFolder()
    {
        using var dialog = new FolderBrowserDialog { SelectedPath = folderBox.Text, Description = "Choose where to create the Gridlock Unreal project" };
        if (dialog.ShowDialog(this) == DialogResult.OK) folderBox.Text = Path.Combine(dialog.SelectedPath, "Gridlock");
    }

    private void DetectEngine()
    {
        detectedEditor = EngineLocator.FindEditor();
        engineLabel.Text = detectedEditor is null
            ? "Unreal Engine 5.4 not detected. Install UE 5.4 or locate UnrealEditor.exe."
            : $"Detected Unreal Engine 5.4: {detectedEditor}";
        installButton.Enabled = detectedEditor is not null;
    }

    private void LocateEditor()
    {
        using var dialog = new OpenFileDialog { Title = "Select the Unreal Engine 5.4 editor", Filter = "Unreal Editor|UnrealEditor.exe|Executable|*.exe" };
        if (dialog.ShowDialog(this) != DialogResult.OK) return;
        if (!EngineLocator.IsUnreal54Editor(dialog.FileName))
        {
            MessageBox.Show(this, "The selected editor does not identify itself as Unreal Engine 5.4.", "Unsupported engine version", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }
        detectedEditor = dialog.FileName;
        engineLabel.Text = $"Selected Unreal Engine 5.4: {detectedEditor}";
        installButton.Enabled = true;
        launchButton.Enabled = installedProject is not null;
    }

    private void FindExistingWorkspace()
    {
        var project = Path.Combine(folderBox.Text, "Gridlock.uproject");
        if (File.Exists(project))
        {
            installedProject = project;
            launchButton.Enabled = detectedEditor is not null;
        }
    }

    private async Task InstallWorkspaceAsync()
    {
        var target = Path.GetFullPath(folderBox.Text.Trim());
        if (string.IsNullOrWhiteSpace(target)) return;
        if (Directory.Exists(target) && Directory.EnumerateFileSystemEntries(target).Any())
        {
            var answer = MessageBox.Show(this, "The chosen folder is not empty. Files will be preserved; existing project files may be replaced. Continue?", "Confirm workspace", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
            if (answer != DialogResult.Yes) return;
        }
        installButton.Enabled = false;
        try
        {
            Directory.CreateDirectory(target);
            await Task.Run(() =>
            {
                using var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("GridlockWorkspace.zip")
                    ?? throw new InvalidOperationException("The workspace payload is missing from this setup executable.");
                using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
                var root = target.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
                foreach (var entry in archive.Entries)
                {
                    var destination = Path.GetFullPath(Path.Combine(target, entry.FullName));
                    if (!destination.StartsWith(root, StringComparison.OrdinalIgnoreCase))
                        throw new InvalidDataException("The workspace archive contains an unsafe path.");
                    if (string.IsNullOrEmpty(entry.Name)) { Directory.CreateDirectory(destination); continue; }
                    Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                    entry.ExtractToFile(destination, overwrite: true);
                }
                var originalProject = Path.Combine(target, "GrandCity.uproject");
                var renamedProject = Path.Combine(target, "Gridlock.uproject");
                if (!File.Exists(originalProject)) throw new FileNotFoundException("The workspace archive has no Unreal project descriptor.");
                File.Move(originalProject, renamedProject, overwrite: true);
                var readme = Path.Combine(target, "README.md");
                File.WriteAllText(Path.Combine(target, "GRIDLOCK_WORKSPACE.txt"),
                    "Gridlock Unreal source workspace\r\n\r\n" +
                    "Open Gridlock.uproject with Unreal Engine 5.4. The C++ project must be compiled from the editor or Visual Studio.\r\n" +
                    "This is source code, not a prebuilt standalone game.\r\n\r\n" +
                    (File.Exists(readme) ? "See README.md and Docs for project notes.\r\n" : ""));
            });
            installedProject = Path.Combine(target, "Gridlock.uproject");
            launchButton.Enabled = detectedEditor is not null;
            MessageBox.Show(this, "Gridlock source files were deployed to the new Unreal workspace. This is a source project, not a packaged game executable.", "Workspace ready", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Setup failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally { installButton.Enabled = detectedEditor is not null; }
    }

    private void LaunchEditor()
    {
        if (detectedEditor is null || installedProject is null || !File.Exists(installedProject)) return;
        System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(detectedEditor, $"\"{installedProject}\"") { UseShellExecute = true });
    }
}

internal static class EngineLocator
{
    public static string? FindEditor()
    {
        var candidates = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var hive in new[] { RegistryHive.LocalMachine, RegistryHive.CurrentUser })
        {
            foreach (var view in new[] { RegistryView.Registry64, RegistryView.Registry32 })
            {
                try
                {
                    using var baseKey = RegistryKey.OpenBaseKey(hive, view);
                    using var key = baseKey.OpenSubKey(@"SOFTWARE\EpicGames\Unreal Engine\5.4");
                    AddEditorPath(candidates, key?.GetValue("InstalledDirectory") as string);
                }
                catch { }
            }
        }

        var roots = new List<string>();
        var programFiles = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
        roots.Add(Path.Combine(programFiles, "Epic Games", "UE_5.4"));
        roots.Add(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Epic Games", "UE_5.4"));
        roots.Add(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Programs", "Epic Games", "UE_5.4"));
        foreach (var drive in DriveInfo.GetDrives().Where(d => d.IsReady))
            roots.Add(Path.Combine(drive.RootDirectory.FullName, "Epic Games", "UE_5.4"));
        foreach (var root in roots) AddEditorPath(candidates, root);

        var launcherDb = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "Epic", "UnrealEngineLauncher", "LauncherInstalled.dat");
        try
        {
            using var json = JsonDocument.Parse(File.ReadAllText(launcherDb));
            if (json.RootElement.TryGetProperty("InstallationList", out var list))
                foreach (var item in list.EnumerateArray())
                    if (item.TryGetProperty("AppName", out var name) && name.GetString()?.Equals("UE_5.4", StringComparison.OrdinalIgnoreCase) == true && item.TryGetProperty("InstallLocation", out var path))
                        AddEditorPath(candidates, path.GetString());
        }
        catch { }
        return candidates.FirstOrDefault(File.Exists);
    }

    public static bool IsUnreal54Editor(string editorPath)
    {
        if (!File.Exists(editorPath) || !Path.GetFileName(editorPath).Equals("UnrealEditor.exe", StringComparison.OrdinalIgnoreCase)) return false;
        var engineRoot = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(editorPath)!, "..", "..", ".."));
        try
        {
            using var json = JsonDocument.Parse(File.ReadAllText(Path.Combine(engineRoot, "Engine", "Build", "Build.version")));
            var root = json.RootElement;
            return root.GetProperty("MajorVersion").GetInt32() == 5 && root.GetProperty("MinorVersion").GetInt32() == 4;
        }
        catch { return false; }
    }

    private static void AddEditorPath(HashSet<string> candidates, string? rootOrExecutable)
    {
        if (string.IsNullOrWhiteSpace(rootOrExecutable)) return;
        var path = rootOrExecutable.Trim().Trim('"');
        if (Directory.Exists(path)) path = Path.Combine(path, "Engine", "Binaries", "Win64", "UnrealEditor.exe");
        if (IsUnreal54Editor(path)) candidates.Add(path);
    }
}
