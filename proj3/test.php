<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
    <head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no"/>
    <link href="http://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.3.0/css/font-awesome.css" rel="stylesheet"  type='text/css'>
    <style>
        h1{
            text-align: center;
        }
        form{
            text-align: center;
        }
        .Context{
            display: block;
            overflow: hidden;
            display: -webkit-box;
            -webkit-box-orient: vertical;
            -webkit-line-clamp: 5; // 限制文字只出現 3 行, 結尾出現省略號
        }
        .input-group{
            text-align: center;
        }
        table{
            margin-left:100px;
            margin-right:100px;
            margin-top: 50px;
            margin-bottom: 50px;
        }
    </style>
    </head>

    <body>
        <h1>Data Engineering</h1>
        <form action="?" method="get">
            <div class="input-group">
                <label for="search">搜尋：</label>
                <input type="text" name="search" value="<?php echo $_GET["search"] ?>">
            </div>
        </form>

        <?php
            header("content-type:text/html;charset=utf-8"); //等同 <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
            header("Cache-Control: no-cache"); //强制浏览器不进行缓存！

            session_start();
            $pageMaxRow = 20;
            $searchKey = "None";
            if (isset($_GET["search"])) {
                $searchKey = $_GET["search"];
            }

            $result = "final.rec";
            $hash_value = hash('sha512', $searchKey);
            $pre_hash_value = shell_exec('tail -n 1 ' . $result);

            // calculate time
            if($searchKey != "None" && $hash_value != $pre_hash_value)
            {
                $startTime = microtime(true);
                $command = "./search ". $searchKey . " > " . $result;
                echo $command;
                $set_charset = 'export LANG=en_US.UTF-8;'; //!!! php
                exec($set_charset.$command, $out);
                $result = "./final.rec";
                $_SESSION['result'] = $result;
                $_SESSION['search'] = $searchKey;
                $endTime = microtime(true);
                $exeTime = $endTime - $startTime;
                echo "<h3>".$exeTime."</h3>";
                file_put_contents($result, $hash_value, FILE_APPEND);
            }

            $page = 1;
            if (isset($_GET["page"]) && $_GET["page"]>=0) {
                $page = $_GET["page"];
            }
            $page = preg_replace('/[^0-9]/', '', stripslashes($page));
            $start_row = ($page - 1) * $pageMaxRow;
            $end_row = $page * $pageMaxRow;
        ?>

        <table class="mainBody" align="center">
			<tbody>
                <?php
                    $handle = fopen($result, "rb");
                    //echo "<div> <table class=\"table table-striped\">";
                    $cnt = 0;
                    while (($line = fgets($handle)) !== false) {
                        if ($cnt >= $start_row && $cnt < $end_row) {
                            $arr = explode("\t", $line);
                            echo '<tr>';
                            echo "<td><a href=" . $arr[1] . ">" . $arr[0] . "</a></td>";
                            echo "<td> Score:" . $arr[2] . "</td>";
                            echo "</tr>";
                            echo "<tr>";
                            echo "<td><p class = 'Context' >" . $arr[3] . '<HR color="#d0d0d0" size="1" width="100%"  align="left">' . "</p></td>";
                            echo "</tr>";
                        }
                        ++$cnt;
                    }
                    //echo "</table></div>";
                ?>
            </tbody>
        </table>

        <?php
            // now $cnt has total line number
            $top_pages = $cnt / $pageMaxRow; // floor
            $url = (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] === 'on' ? "https" : "http") . "://$_SERVER[HTTP_HOST]" . explode('?',  $_SERVER['REQUEST_URI'], 2 )[0];
            $start_page = $page - 4;
            $end_page = $page + 5;
            if($start_page < 1)
            {
                $start_page = 1;
            }
            if($end_page > $top_pages)
            {
                $end_page = $top_pages;
            }

            echo "<div align=\"center\">";
            if($start_page != 1)
            {
                $newurl = $url . "?search=" . rawurlencode($searchKey) . "&page=1";
                echo '<a href="' . $newurl . '"><button type="button" class="btn btn-default"; style="margin-left:10px;">' . 1 . '</button></a>';
            }
            for ($i = $start_page; $i < $end_page; ++$i) {
                $newurl = $url . "?search=" . rawurlencode($searchKey) . "&page=" . $i;
                echo '<a href="' . $newurl . '"><button type="button" class="btn btn-default"; style="margin-left:10px;">' . ($i) . '</button></a>';
            }
            if($end_page != $top_pages)
            {
                $newurl = $url . "?search=" . rawurlencode($searchKey) . "&page=" . intval($top_pages);
                echo '&nbsp;&nbsp;>> <a href="' . $newurl . '"><button type="button" class="btn btn-default"; style="margin-left:10px;">' . intval($top_pages) . '</button></a>';
            }
            echo "</div>";
        ?>
    </body>
</html>
