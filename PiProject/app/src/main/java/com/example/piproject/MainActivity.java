package com.example.piproject;


import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.media.MediaPlayer;
import android.media.RingtoneManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Vibrator;
import android.util.Log;
import android.util.PrintWriterPrinter;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;



import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.Socket;
import java.net.URL;
import java.net.URLConnection;
import java.net.UnknownHostException;



public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    // 新增：单帧轮询
    private final Handler frameHandler = new Handler(Looper.getMainLooper());
    private Runnable frameRunnable;
    private String snapshotUrl;           // http://192.168.x.x:8080/?action=snapshot
    OutputStream outputStream;
    String buf;
    Socket socket;
    //int mScreenHeight = Resources.getSystem().getDisplayMetrics().heightPixels;
    //int mScreenWidth = Resources.getSystem().getDisplayMetrics().widthPixels;

    String string;
    int mPort;
    String host;
    WebView webView;
    WebView mWebView;

    MediaPlayer mp = new MediaPlayer();
    Vibrator vibrator;
    Button button;
    Handler handler;
    Runnable refreshRunnable;
    //private String urlFlask = "http://192.168.137.136:5000/person";
    String host2 = "172.22.8.16";
    String port2 = "5000";
    private String urlFlask;
    //private String urlFlask = "http://172.22.8.16:5000/person";
    String channelId = "test";

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
//        setContentView(R.layout.activity_main);
        setContentView(R.layout.activity_main);

        //设置屏幕为横屏, 设置后会锁定方向
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
//        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);//设置屏幕为竖屏, 设置后会锁定方向

        initSocket();

        String url = "http://192.168.137.171:8080/?action=stream";
//        String url="http://"+host+":8080/?action=stream";
//        Log.d("MainActivity", "onCreate: host="+host);
//        WebView webView = findViewById(R.id.webview);
//        webView.loadUrl(url);

        Button turnLeft = findViewById(R.id.turn_left);
        Button turRight = findViewById(R.id.turn_right);
        Button front = findViewById(R.id.front);
        Button back = findViewById(R.id.back);
        Button stop = findViewById(R.id.stop);

        Button up = findViewById(R.id.up);
        Button down = findViewById(R.id.down);
        Button left = findViewById(R.id.left);
        Button right = findViewById(R.id.right);

        // 为按钮设置监听器
        up.setOnClickListener(this);
        down.setOnClickListener(this);
        left.setOnClickListener(this);
        right.setOnClickListener(this);
        turnLeft.setOnClickListener(this);
        turRight.setOnClickListener(this);
        front.setOnClickListener(this);
        back.setOnClickListener(this);
        stop.setOnClickListener(this);

        mWebView = findViewById(R.id.mwebview);
        button = findViewById(R.id.off);


        handler = new Handler();
        refreshRunnable = new Runnable() {
            @Override
            public void run() {
                // 刷新WebView页面
                mWebView.reload();
                Log.d(TAG, "TAG run: webView.reload()");
                // 定时执行下一次刷新
                handler.postDelayed(this, 100); // 每5秒刷新一次
            }
        };
        handler.postDelayed(refreshRunnable, 1);

    }

    /**
     * 活动销毁时执行
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacks(refreshRunnable);   // 原来就有
        frameHandler.removeCallbacks(frameRunnable); // 新增
    }

    /**
     * 点击不同的按钮发送不同的指令到服务端
     * @param view View
     */
//    @Override
//    public void onClick(View view) {
//        switch (view.getId()) {
//            case R.id.up:
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("ONK");
//                    }
//                }).start();
//                break;
//            case R.id.down:
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("ONJ");
//                    }
//                }).start();
//                break;
//            case R.id.left:
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("ONL");
//                    }
//                }).start();
//                break;
//            case R.id.right:
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("ONI");
//                    }
//                }).start();
//                break;
//            case R.id.turn_left:
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("ONC");
//                    }
//                }).start();
//                break;
//            case R.id.turn_right:
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("OND");
//                    }
//                }).start();
//                break;
//            case R.id.front:
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("ONA");
//                    }
//                }).start();
//                break;
//            case R.id.back:
//                //发送数据
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("ONB");
//                    }
//                }).start();
//                break;
//            case R.id.stop:
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        initSendCommand("ONE");
//                    }
//                }).start();
//                break;
//            default:
//                break;
//        }
//    }
    @Override
    public void onClick(View view) {
        int viewId = view.getId();

        if (viewId == R.id.up) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("ONK");
                }
            }).start();
        } else if (viewId == R.id.down) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("ONJ");
                }
            }).start();
        } else if (viewId == R.id.left) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("ONL");
                }
            }).start();
        } else if (viewId == R.id.right) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("ONI");
                }
            }).start();
        } else if (viewId == R.id.turn_left) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("ONC");
                }
            }).start();
        } else if (viewId == R.id.turn_right) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("OND");
                }
            }).start();
        } else if (viewId == R.id.front) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("ONA");
                }
            }).start();
        } else if (viewId == R.id.back) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("ONB");
                }
            }).start();
        } else if (viewId == R.id.stop) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    initSendCommand("ONE");
                }
            }).start();
        }
    }


    private void initSocket() {
        //连接socket
        runOnUiThread(new Runnable() {
            @Override
            public void run() {

                LayoutInflater factory = LayoutInflater.from(MainActivity.this);
                final View textEntryView = factory.inflate(R.layout.dialog, null);
                final EditText input = textEntryView.findViewById(R.id.editTextName);
                final EditText input2 = textEntryView.findViewById(R.id.editTextNum);
                input.setText("192.168.137.252 2001");  // 默认 小车IP
                input2.setText("192.168.137.252 8080");  // 默认   监视器IP
                AlertDialog.Builder ad1 = new AlertDialog.Builder(MainActivity.this);
                ad1.setTitle("连接...");
//                ad1.setIcon(android.R.drawable.ic_dialog_info);
                ad1.setView(textEntryView);
                ad1.setPositiveButton("是", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int i) {

                        // 树莓派小车的IP与端口号
                        string = input.getText().toString();
                        Log.d("MainActivity", "onClick: string:" + string);
                        int space = string.indexOf(' ');
                        host = string.substring(0, space);
                        String port = string.substring(space + 1, string.length());
                        Log.d("MainActivity", "host:" + host + " port:" + port);
                        //socket = new Socket("192.168.137.171", 2001);
                        mPort = Integer.parseInt(port);
                        Toast.makeText(MainActivity.this, "小车:"+host+":"+port, Toast.LENGTH_SHORT).show();

                        // 实时视频对应的IP和端口号
                        string = input2.getText().toString();
                        int space2 = string.indexOf(' ');
                        host2 = string.substring(0, space2);
                        port2 = string.substring(space2 + 1, string.length());
                        //String url = "http://172.22.8.16:5000";
                        String url = "http://" + host2 + ":8080/?action=stream";

                        Log.d(TAG, "onClick: url"+url);
                        Toast.makeText(MainActivity.this, "检测:"+url, Toast.LENGTH_SHORT).show();
                        //String url = "http://192.168.137.136:5000/video_feed";
                        webView = findViewById(R.id.webview);
                        webView.loadUrl(url);

                        urlFlask = "http://"+host2+":"+port2+"/person";
                        Log.d(TAG, "initWebView: urlFlask:"+urlFlask);
                        Toast.makeText(MainActivity.this, "urlFlask="+urlFlask, Toast.LENGTH_SHORT).show();
                        initWebView();

                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                try {
                                    socket = new Socket(host, mPort);
                                    outputStream = socket.getOutputStream();
                                } catch (IOException e) {
                                    e.printStackTrace();
                                    // 回到主线程弹出提示框
                                    runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            new AlertDialog.Builder(MainActivity.this)
                                                    .setTitle("连接失败")
                                                    .setMessage("无法连接到小车，请检查 IP、端口及网络后重试！")
                                                    .setPositiveButton("确定", null)
                                                    .show();
                                        }
                                    });
                                }
                            }
                        }).start();

                    }
                });
                ad1.setNegativeButton("否", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int i) {
                        finish();
                    }
                });
                ad1.show();// 显示对话框
            }
        });
    }

    private void initSendCommand(String command) {
        if (outputStream == null) {
            Log.e(TAG, "Socket 未连接，忽略指令：" + command);
            return;
        }
        new Thread(() -> {
            try {
                outputStream.write(command.getBytes());
                outputStream.flush();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }).start();
    }

    private void initWebView() {
        snapshotUrl = "http://" + host2 + ":8080/?action=snapshot";

        WebSettings settings = mWebView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            settings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
        }

        // 让 WebView 只负责显示我们注入的 html，不再直接访问 Flask
        mWebView.setWebViewClient(new WebViewClient());

        // 简单的 html：自动刷新单帧
        String html = "<html><body style='margin:0;padding:0;background:#000;'>" +
                "<img id='cam' style='width:100%;height:auto;display:block;'>" +
                "<script>" +
                "  var img = document.getElementById('cam');" +
                "  function reload() {" +
                "    img.src = '" + snapshotUrl + "&t=' + Date.now();" +
                "  }" +
                "  reload();" +
                "  setInterval(reload, 100);" +   // 100 ms ≈ 10 fps
                "</script>" +
                "</body></html>";

        mWebView.loadDataWithBaseURL(null, html, "text/html", "utf-8", null);
    }


    /**
     * AndroidJavaScript
     * 本地与h5页面交互的js类，这里写成内部类了
     * returnAndroid方法上@JavascriptInterface一定不能漏了
     */
    private class AndroidJavaScript {
        Context mContxt;

        public AndroidJavaScript(Context mContxt) {
            this.mContxt = mContxt;
        }

        /**
         * 接收来自前端网页的信息
         * @param name String
         * @throws IOException IOException
         */
        @JavascriptInterface
        public void returnAndroid(String name) throws IOException {
            if (name.isEmpty()) {
                return;
            }

            Log.d(TAG, "TAG returnAndroid: name=" + name);

            // 如果检测到有人入侵
            if ("person".equals(name)) {
                Log.d(TAG, "TAG person.equals(name)");

                // 响铃
                mp.setDataSource(MainActivity.this, RingtoneManager
                        .getDefaultUri(RingtoneManager.TYPE_RINGTONE));//铃声
                if (!mp.isPlaying()) {
                    mp.prepare();
                    mp.start();
                    mp.setLooping(false); //循环播放
                }
                Log.d(TAG, "TAG finish mp");

                //取得震动服务的句柄
                vibrator = (Vibrator) getSystemService(VIBRATOR_SERVICE);
                //按照指定的模式去震动。
                //	vibrator.vibrate(1000);
                //数组参数意义：第一个参数为等待指定时间后开始震动，震动时间为第二个参数。后边的参数依次为等待震动和震动的时间
                //第二个参数为重复次数，-1为不重复，0为一直震动
                vibrator.vibrate(new long[]{1000, 1000}, 0);
                Log.d(TAG, "TAG finish vibrator");


                // 通知
                // 1. 创建一个通知(必须设置channelId)
                Notification.BigPictureStyle bigPictureStyle = new Notification.BigPictureStyle()
                        .setBigContentTitle("小心")
                        .setSummaryText("有危险发生");

                Notification notification = null;
                NotificationManager notificationManager = null;
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
                    notification = new Notification.Builder(MainActivity.this, channelId)
                            .setContentTitle("注意！！！")
                            .setContentText("有人入侵")
                            .setWhen(System.currentTimeMillis())
                            .setSmallIcon(R.drawable.ic_launcher_foreground)
                            .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.drawable.ic_launcher_background))   //设置大图标
                            .setStyle(bigPictureStyle)
                            .build();
                    // 2. 获取系统的通知管理器
                    notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
                    // 3. 创建NotificationChannel(这里传入的channelId要和创建的通知channelId一致，才能为指定通知建立通知渠道)
                    NotificationChannel channel = new NotificationChannel(channelId, "prescriptivist", NotificationManager.IMPORTANCE_DEFAULT);
                    notificationManager.createNotificationChannel(channel);
                }
                // 4. 发送通知
                notificationManager.notify(1123, notification);
                Log.d(TAG, "TAG finish notification");

                button.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        mp.stop();
                        vibrator.cancel();
                        //mWebView.loadUrl(url);
                    }
                });
            }

            //Toast.makeText(MainActivity.this, name, Toast.LENGTH_SHORT).show();
            Log.d(TAG, "TAG returnAndroid: 从js传来的数据：" + name);
        }
    }


}