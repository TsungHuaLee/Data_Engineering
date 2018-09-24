<!DOCTYPE html>
<html>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
    <link href="http://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.3.0/css/font-awesome.css" rel="stylesheet"  type='text/css'>
        <body>

            <form action="?" method="get">
                <div class="input-group mb-3">
                    <label for="search">關鍵字搜尋：</label>
                    <input type="search" class="form-control" id="search" name="search" placeholder="Search..">
                    <span class="input-group-btn">
                        <button type="submit"><i class="fa fa-search"></i></button>
                    </span>
                </div>
            </form>

            <?php
            header("content-type:text/html;charset=utf-8"); //等同 <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
            header("Cache-Control: no-cache"); //强制浏览器不进行缓存！

            session_start();
            $pageMaxRow = 100;
            $searchKey = "None";
            if (isset($_GET["search"])) {
                $searchKey = $_GET["search"];
            }

            $page = 0;
            if (isset($_GET["page"]) && $_GET["page"]>=0) {
                $page = $_GET["page"];
            }
            //$page = preg_replace('/[^0-9]/', '', stripslashes($page));
            $start_row = $page * $pageMaxRow;
            $end_row = ($page+1) * $pageMaxRow;


            $tmp_result = tempnam("./tmp", "search_sentence_");
            $command = "grep \"^" . $searchKey . "\" ./sentences.txt > " . $tmp_result;
            exec($command, $outputs, $return_status);
            $_SESSION['tmp_result'] = $tmp_result;
            $_SESSION['search'] = $searchKey;

            $handle = fopen($tmp_result, "r");
            echo "<div> <table class=\"table table-striped\">";
            echo "<tr> <th> 文章 </th><th> | 內文 </th> </tr>";
            $cnt = 0;
            while (($line = fgets($handle)) !== false) {
                if ($cnt >= $start_row && $cnt < $end_row) {
                    $arr = explode("\t", $line);
                    echo "<tr>";
                    echo "<td><a href=" . $arr[2] . ">" . $arr[1] . "</a></td>";
                    echo "<td>" . $arr[0] . "</td>";
                    echo "</tr>";
                }
                ++$cnt;
            }
            echo "</table></div>";
            // now $cnt has total line number
            $tot_pages = $cnt / $pageMaxRow; // floor
            $url = (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] === 'on' ? "https" : "http") . "://$_SERVER[HTTP_HOST]" . explode('?',  $_SERVER['REQUEST_URI'], 2 )[0];
            echo "<div align=\"center\">";
            for ($i=0; $i<$tot_pages; ++$i) {
                $newurl = $url . "?search=" . $searchKey . "&page=" . $i;
                echo "<a href=\"" . $newurl . "\"><button type=\"button\" class=\"btn btn-default\">" . ($i+1) . "</button></a>";
            }
            echo "</div>";
            ?>
        </body>
</html>
