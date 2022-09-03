package com.abc.memtool;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    EditText searchPackage;
    EditText searchValue;
    TextView resultView;
    Button searchButton;
    Button filterButton;
    Button editButton;

    @RequiresApi(api = Build.VERSION_CODES.N)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // 拷贝可执行文件到/data/data/包名/executable下
        CopyElfs copyElfs = new CopyElfs(getBaseContext());
        copyElfs.copyAll2Data();
        // 定义菜单
        searchPackage = findViewById(R.id.text_search_package);
        searchValue = findViewById(R.id.text_search_value);
        resultView = findViewById(R.id.text_result);
        searchButton = findViewById(R.id.button_search);
        searchButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String cmd = copyElfs.executableFilePath + "/memtool s "+searchValue.getText().toString()+" "+searchPackage.getText().toString();
                String result = RootCommand(cmd);
                resultView.setText(result);
            }
        });

        filterButton = findViewById(R.id.button_filter);
        filterButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String resultViewString = resultView.getText().toString();
                String cmd = copyElfs.executableFilePath + "/memtool f "+searchValue.getText().toString()+" "+searchPackage.getText().toString()+" "+resultViewString.replaceAll("\n"," ");
                String result = RootCommand(cmd);
                resultView.setText(result);
            }
        });
        editButton = findViewById(R.id.button_edit);
        editButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String resultViewString = resultView.getText().toString();
                String cmd = copyElfs.executableFilePath + "/memtool e "+searchValue.getText().toString()+" "+searchPackage.getText().toString()+" "+resultViewString.replaceAll("\n"," ");
                String result = RootCommand(cmd);
                Toast.makeText(MainActivity.this, result, Toast.LENGTH_SHORT).show();
            }
        });
    }

    public static String RootCommand(String command) {
        String allResult = "";
        Process process = null;
        DataOutputStream os = null;
        try {
            process = Runtime.getRuntime().exec("su");
            os = new DataOutputStream(process.getOutputStream());
            os.writeBytes(command + "\n");
            os.writeBytes("exit\n");
            os.flush();
            process.waitFor();
            BufferedReader br = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String result;
            while ((result = br.readLine()) != null) {
                allResult += result + "\n";
            }
        } catch (Exception e) {
            Log.d("*** DEBUG ***", "ROOT REE" + e.getMessage());
            return null;
        } finally {
            try {
                if (os != null) {
                    os.close();
                }
                process.destroy();
            } catch (Exception e) {
            }
        }
        Log.d("*** DEBUG ***", "Root SUC ");
        return allResult;
    }

    class CopyElfs {
        String tag = "StackOF:";
        Context ct;
        String appFileDirectory, executableFilePath;
        AssetManager assetManager;
        List resList;
        String cpuType;
        String[] assetsFiles = {
                "memtool"
        };

        @RequiresApi(api = Build.VERSION_CODES.N)
        CopyElfs(Context c) {
            ct = c;
            //        appFileDirectory = ct.getFilesDir().getPath();
            appFileDirectory = "/data/data/" + ct.getPackageName();
            executableFilePath = appFileDirectory + "/executable";
            Log.e(tag, "cpu type:" + cpuType);
            //        cputype = Build.SUPPORTED_ABIS[0];
            cpuType = Build.CPU_ABI;
            Log.e(tag, "cpu type:" + cpuType);
            assetManager = ct.getAssets();
            try {
                resList = Arrays.asList(ct.getAssets().list(cpuType + "/"));
                Log.d(tag, "get assets list:" + resList.toString());
            } catch (IOException e) {
                Log.e(tag, "error list assets folder:", e);
            }
        }

        boolean resFileExist(String filename) {
            File f = new File(executableFilePath + "/" + filename);
            if (f.exists())
                return true;
            return false;
        }

        void copyFile(InputStream in, OutputStream out) {
            try {
                byte[] buf = new byte[1024];
                int len;
                while ((len = in.read(buf)) > 0) {
                    out.write(buf, 0, len);
                }
            } catch (IOException e) {
                Log.e(tag, "failed to read/write asset file: ", e);
            }
        }

        ;

        private void copyAssets(String filename) {
            InputStream in = null;
            OutputStream out = null;
            Log.d(tag, "attempting to copy this file: " + filename);
            try {
                in = assetManager.open(cpuType + "/" + filename);
                File outfile = new File(executableFilePath, filename);
                out = new FileOutputStream(outfile);
                copyFile(in, out);
                in.close();
                in = null;
                out.flush();
                out.close();
                out = null;
            } catch (IOException e) {
                Log.e(tag, "failed to copy asset file: " + filename, e);
            }
            Log.d(tag, "copy success: " + filename);
        }

        void copyAll2Data() {
            int i;
            File folder = new File(executableFilePath);
            if (!folder.exists()) {
                folder.mkdir();
            }
            for (i = 0; i < assetsFiles.length; i++) {
                if (!resFileExist(assetsFiles[i])) {
                    copyAssets(assetsFiles[i]);
                    File execfile = new File(executableFilePath + "/" + assetsFiles[i]);
                    execfile.setExecutable(true);
                }
            }
        }

        String getExecutableFilePath() {
            return executableFilePath;
        }
    }

}