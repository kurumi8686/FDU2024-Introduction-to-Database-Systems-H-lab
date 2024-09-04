from flask import request, render_template, redirect, url_for, flash, Flask, session
import pymysql
import random

# 创建Flask应用实例
app = Flask(
    __name__,
    template_folder="templates",
    static_url_path="/static",
    static_folder="static",
)
# set key
app.secret_key = "it_just_a_secret_key."

# in your environment, you should change the info to adapt it.
# host and port should change according to your computer, and so as passwd, db
def mysql_connect():
    return pymysql.connect(
        host="127.0.0.1",
        port=3306,
        user="root",
        passwd="kurumi",
        db="userdb",
        charset="utf8",
    )

# 定义路由和对应的视图函数
# index
@app.route("/")
def index():
    conn = mysql_connect()
    cursor = conn.cursor()
    sql = "select * from goods"
    cursor.execute(sql)
    result = cursor.fetchall()
    result1 = []
    for i in result:
        # whether has image?
        i0 = i[8]
        if i0:
            i1 = str(i0).split("/")
            i2 = i1[1]
            image = "../static/images/" + i2
            j = {"goods_name": i[5], "price": i[7], "image": image}
            result1.append(j)
        else:
            j = {"goods_name": i[5], "price": i[7], "image": image}
            result1.append(j)
    result2 = random.sample(result1, 9)
    return render_template("index.html", result=result2)
# index after login
@app.route("/index_after", methods=["POST", "GET"])
def index_after():
    conn = mysql_connect()
    cursor = conn.cursor()
    sql = "select * from goods"
    cursor.execute(sql)
    result = cursor.fetchall()
    result1 = []
    for i in result:
        i0 = i[8]
        if i0:
            i1 = str(i0).split("/")
            i2 = i1[1]
            image = "../static/images/" + i2
            j = {"goods_name": i[5], "price": i[7], "image": image}
            result1.append(j)
        else:
            j = {"goods_name": i[5], "price": i[7], "image": image}
            result1.append(j)
    result2 = random.sample(result1, 9)
    return render_template("index_after.html", result=result2)
@app.route("/register", methods=["GET", "POST"])
def register():
    if request.method == "POST":
        conn = mysql_connect()
        cursor = conn.cursor()
        username = request.form["username"]
        password = request.form["password"]
        password_again = request.form["password_again"]
        phone_number = request.form["phone_number"]
        if int(len(phone_number)) > 12:
            flash("请输入正确的手机号！", category="error")
            return redirect(url_for("register"))
        if password == password_again:
            # 保证不重名
            cursor.execute("select name from users where name=%s",[username,],)
            result = cursor.fetchone()
            # 为了保证在应用程序、数据库或系统出现错误后，数据库能够被还原，以保证数据库的完整性，所以需要进行回滚。
            # 回滚(rollback)就是在事务提交之前将数据库数据恢复到事务修改之前数据库数据状态。
            if result:
                flash("该用户名太热门了喵，请换一个吧~", category="error")
                return redirect(url_for("register"))
            elif '"' in username or "'" in username:
                flash("格式错误（不允许带有英文引号）！", category="error")
                return redirect(url_for("register"))
            else:
                sql = "insert into users(name,password,phone_number) values('{}','{}','{}')".format(
                    username, password_again, phone_number
                )
                cursor.execute(sql)
                conn.commit()
                flash("注册成功！请登录", category="info")
                # 注册成功之后跳转到登录页面
                return redirect(url_for("login"))
        else:
            flash("两次密码输入不一致！", category="error")
            return redirect(url_for("register"))

    else:
        return render_template("register.html")
# 在用户登录时，将账户信息存储在session中（注意要设置密匙！！）
# check for info of this account
def check_login(username, password):
    conn = mysql_connect()
    cursor = conn.cursor()
    sql = "SELECT name, password FROM users WHERE name='{}' AND password='{}'".format(
        username, password
    )
    cursor.execute(sql)
    result = cursor.fetchone()
    cursor.close()
    conn.close()
    if result:
        return result
    else:
        return
# result为一个元组('Rozwal', 'Kurumi')或None
@app.route("/login", methods=["POST", "GET"])
def login():
    if request.method == "POST":
        username = request.form["username"]
        password = request.form["password"]
        if check_login(username, password):
            flash("登录成功！", category="info")
            # 将用户名存储在session中
            session["username"] = username
            # 重新定向到主页（登陆后）
            return redirect(url_for("index_after"))
        else:
            flash("用户名或密码输入错误！", category="error")
            return redirect(url_for("login"))
    else:
        return render_template("login.html")
@app.route("/logout")
def logout():
    # erase username from session
    session.pop("username", None)
    flash("成功退出登录！", category="info")
    return redirect(url_for("index"))
# change personal_info
@app.route("/mend_info", methods=["GET", "POST"])
def mend_info():
    if request.method == "POST":
        conn = mysql_connect()
        cursor = conn.cursor()
        username_origin = session.get("username")
        sql = "select * from users where name = '{}'".format(username_origin)
        cursor.execute(sql)
        result = cursor.fetchone()
        uid_origin = result[3]
        sex_origin = result[4]
        campus_origin = result[5]
        intro_origin = result[6]

        username = request.form["username"]
        intro = request.form["intro"]
        campus = request.form["campus"]
        uid = request.form["uid"]
        sex = request.form["sex"]

        if not intro:
            intro = intro_origin

        if not uid:
            uid = uid_origin

        if not sex:
            sex = sex_origin

        if not campus:
            campus = campus_origin

        if username:
            session["username"] = username
            # 保证不重名
            cursor.execute("select name from users where name=%s",[username,],)
            result = cursor.fetchone()
            # 为了保证在应用程序、数据库或系统出现错误后，数据库能够被还原，以保证数据库的完整性，所以需要进行回滚。
            # 回滚(rollback)就是在事务提交之前将数据库数据恢复到事务修改之前数据库数据状态。
            if result:
                flash("该用户名太热门了喵，请换一个吧~", category="error")
                return redirect(url_for("mend_info"))
            elif '"' in username or "'" in username:
                flash("格式错误（不允许带有英文引号）！", category="error")
                return redirect(url_for("mend_info"))
            else:
                sql = "update users set name='{}' where name='{}'".format(
                    username, username_origin
                )
                cursor.execute(sql)
                conn.commit()
                sql = "update goods set seller='{}' where seller='{}'".format(
                    username, username_origin
                )
                cursor.execute(sql)
                conn.commit()
                sql = "update users set campus='{}' where campus='{}'".format(
                    campus, campus_origin
                )
                cursor.execute(sql)
                conn.commit()
                sql = "update users set brief_intro='{}' where brief_intro='{}'".format(
                    intro, intro_origin
                )
                cursor.execute(sql)
                conn.commit()
                sql = "update users set sex='{}' where sex='{}'".format(sex, sex_origin)
                cursor.execute(sql)
                conn.commit()
                sql = "update users set uid='{}' where uid='{}'".format(uid, uid_origin)
                cursor.execute(sql)
                conn.commit()
                cursor.close()
                conn.close()
                flash("修改成功！请您重新登录", category="info")
                # 注册成功之后跳转到登录页面
                return redirect(url_for("login"))

        if not username:
            sql = "update users set campus='{}' where name='{}'".format(
                campus, username_origin
            )
            cursor.execute(sql)
            conn.commit()
            sql = "update users set brief_intro='{}' where name='{}'".format(
                intro, username_origin
            )
            cursor.execute(sql)
            conn.commit()
            sql = "update users set sex='{}' where name='{}'".format(
                sex, username_origin
            )
            cursor.execute(sql)
            conn.commit()
            sql = "update users set uid='{}' where name='{}'".format(
                uid, username_origin
            )
            cursor.execute(sql)
            conn.commit()
            cursor.close()
            conn.close()
            flash("修改成功！请您重新登录", category="info")
            # 注册成功之后跳转到登录页面
            return redirect(url_for("login"))

    else:
        return render_template("mend_info.html")
# show personal information
@app.route("/personal_info", methods=["POST", "GET"])
def personal_info():
    conn = mysql_connect()
    cursor = conn.cursor()
    # 在需要使用账户信息的页面或视图函数中，可以通过session对象来获取保存在session中的数据
    username = session.get("username")
    sql = "select * from users where name = '{}'".format(username)
    cursor.execute(sql)
    result = cursor.fetchone()
    db_username = result[1]
    uid = result[3]
    sex = result[4]
    campus = result[5]
    intro = result[6]
    i0 = result[8]
    if i0:
        i1 = str(i0).split("/")
        i2 = i1[1]
        image = "../static/images/" + i2
        return render_template(
            "personal_info.html",
            username=db_username,
            uid=uid,
            sex=sex,
            campus=campus,
            intro=intro,
            image=image,
        )
    else:
        return render_template(
            "personal_info.html",
            username=db_username,
            uid=uid,
            sex=sex,
            campus=campus,
            intro=intro,
        )
# show personal trade logging
@app.route("/personal_trade", methods=["POST", "GET"])
def personal_trade():
    conn = mysql_connect()
    cursor = conn.cursor()
    username = session.get("username")
    cursor.execute(
        "select goods_name,price from goods where buyer='{}'".format(username)
    )
    result = cursor.fetchall()
    purchase_goods = []
    for i in result:
        if i != ("", ""):
            purchase_goods.append(i)
    cursor.execute(
        "select goods_name,price from goods where seller='{}'".format(username)
    )
    result = cursor.fetchall()
    sell_goods = []
    for i in result:
        if i != ("", ""):
            sell_goods.append(i)

    sql = "select * from users where name = '{}'".format(username)
    cursor.execute(sql)
    result = cursor.fetchone()
    i0 = result[8]
    if i0:
        i1 = str(i0).split("/")
        i2 = i1[1]
        image = "../static/images/" + i2
        return render_template(
            "personal_trade.html",
            username=username,
            image=image,
            purchase_goods=purchase_goods,
            sell_goods=sell_goods,
        )
    else:
        return render_template(
            "personal_trade.html",
            username=username,
            purchase_goods=purchase_goods,
            sell_goods=sell_goods,
        )
# report
@app.route("/report", methods=["POST", "GET"])
def report():
    if request.method == "POST":
        conn = mysql_connect()
        cursor = conn.cursor()
        reporter = session.get("username")
        username = request.form["username"]
        reason = request.form["reason"]

        cursor.execute("select name from users where name='{}'".format(username))
        result = cursor.fetchone()
        if not result:
            flash("用户名输入错误！请准确输入", category="error")
            return redirect(url_for("report"))
        if result:
            sql = "insert into report(username,reason,reporter) values('{}','{}','{}')".format(
                username, reason, reporter
            )
            cursor.execute(sql)
            conn.commit()
            cursor.close()
            conn.close()
            flash("举报成功，请等待我们的处理。", category="info")
            return redirect(url_for("index_after"))

    else:
        return render_template("report.html")

# feedback to developer
@app.route("/feedback", methods=["POST", "GET"])
def feedback():
    if request.method == "POST":
        feedbacker = session.get("username")
        conn = mysql_connect()
        cursor = conn.cursor()
        # get the account info from session
        choice = request.form["choice"]
        contact = request.form["contact"]
        contents = request.form["contents"]
        if not choice or not contact:
            flash("请正确留下您的联系方式！", category="info")
            return render_template("feedback.html")
        elif not contents:
            flash("请输入反馈内容。", category="info")
            return render_template("feedback.html")
        else:
            sql = "insert into feedback(feedbacker,contact,contents,choice) values('{}','{}','{}','{}')".format(
                feedbacker, contact, contents, choice
            )
            cursor.execute(sql)
            conn.commit()
            cursor.close()
            conn.close()
            flash("我们已收到您的反馈！", category="info")
            return redirect(url_for("index_after"))
    else:
        return render_template("feedback.html")

# publish your goods
# you may have to change the path to save those goods' images.
static_images_filepath = r"C:\Users\12980\PycharmProjects\web_development_study\app\static\images/"

@app.route("/goodsupload", methods=["GET", "POST"])
def goodsupload():
    if request.method == "POST":
        conn = mysql_connect()
        cursor = conn.cursor()
        seller = session.get("username")
        # 获取表单中用户提交的账户信息
        goods_name = request.form["goods_name"]
        goods_type = request.form["type"]
        price = request.form["price"]
        campus = request.form["campus"]
        description = request.form["brief_intro"]
        wx = request.form["wx"]
        image = request.files["image"]
        if image:
            filename = image.filename
            filepath = (
                static_images_filepath + filename
            )
            image.save(filepath)
            sql = (
                "insert into goods(goods_name,description,type,price,campus,seller,image) "
                "values('{}','{}','{}','{}','{}','{}','{}')".format(
                    goods_name, description, goods_type, price, campus, seller, filepath
                )
            )
            cursor.execute(sql)
            conn.commit()
            sql = "update users set wx='{}' where name='{}';".format(wx, seller)
            cursor.execute(sql)
            conn.commit()
            cursor.close()
            conn.close()
            flash("发布成功！", category="info")
            return redirect(url_for("index_after"))
        else:
            sql = (
                "insert into goods(goods_name,description,type,price,campus,seller) "
                "values('{}','{}','{}','{}','{}','{}')".format(
                    goods_name, description, goods_type, price, campus, seller
                )
            )
            cursor.execute(sql)
            conn.commit()
            cursor.close()
            conn.close()
            flash("发布成功！", category="info")
            return redirect(url_for("index_after"))

    else:
        return render_template("goodsupload.html")
# 上传图片函数（更换头像函数）
@app.route("/imageupload", methods=["GET", "POST"])
def imageupload():
    if request.method == "POST":
        conn = mysql_connect()
        cursor = conn.cursor()
        username = session.get("username")
        image = request.files["image"]
        if image:
            filename = image.filename
            filepath = (
                static_images_filepath + filename
            )
            image.save(filepath)
            sql = "update users set image='{}' where name='{}'".format(
                filepath, username
            )
            cursor.execute(sql)
            conn.commit()
            cursor.close()
            conn.close()
            flash("图片上传成功！")
            return redirect(url_for("index_after"))
        else:
            return "图片上传失败……"

    else:
        return render_template("imageupload.html")
@app.route("/goodsdownload", methods=["GET", "POST"])
def goodsdownload():
    if request.method == "POST":
        username = session.get("username")
        conn = mysql_connect()
        cursor = conn.cursor()
        goodsname = request.form["goodsname"]
        # 使用参数化查询，防止SQL注入攻击
        sql = "select * from goods where seller = %s and goods_name = %s"
        cursor.execute(sql, (username, goodsname))
        conn.commit()
        res = cursor.fetchall()
        if res:
            # 使用参数化查询，防止SQL注入攻击
            sql = "delete from goods where seller = %s and goods_name = %s"
            cursor.execute(sql, (username, goodsname))
            conn.commit()
            cursor.close()
            conn.close()
            flash("商品下架成功！", category="info")
            return redirect(url_for("index_after"))
        else:
            cursor.close()
            conn.close()
            flash("您未发布该商品，请正确输入商品名", category="info")
            return redirect(url_for("goodsdownload"))
    else:
        return render_template("goodsdownload.html")


# search goods function
@app.route("/search", methods=["POST", "GET"])
def search():
    if request.method == "POST":
        conn = mysql_connect()
        cursor = conn.cursor()
        result = request.form["result"]
        sql = "select * from goods where goods_name like '%{}%' or description like '%{}%'".format(
            result, result
        )
        cursor.execute(sql)
        result1 = cursor.fetchall()
        cursor.close()
        conn.close()
        # 将搜索结果放入session会话中储存，后续content中取出，记得消除原有的缓存
        session["result"] = result1
        return redirect(url_for("outcome"))
    else:
        return render_template("search.html")
# show the search result function
@app.route("/outcome", methods=["POST", "GET"])
def outcome():
    result = session.get("result")
    number = len(result)
    productdata = []
    cur = 0
    for i in result:
        cur = cur + 1
        j = i[5] + "||" + i[6] + "||" + i[7] + "元"
        k = {"id": cur, "goods_name": j}
        productdata.append(k)
    return render_template("outcome.html", number=number, productdata=productdata)
# show the goods' details
@app.route("/content/<id>", methods=["POST", "GET"])
def content(id):
    result = session.get("result")
    content = result[int(id) - 1]
    description = content[1]
    type = content[2]
    seller = content[4]
    goods_name = content[5]
    campus = content[6]
    price = content[7]
    i0 = content[8]
    if i0:
        i1 = str(i0).split("/")
        i2 = i1[1]
        image = "../static/images/" + i2
        conn = mysql_connect()
        cursor = conn.cursor()
        sql = "select wx from users where name='{}'".format(seller)
        cursor.execute(sql)
        wx = cursor.fetchone()
        for wx1 in wx:
            wx = wx1
        return render_template(
            "content.html",
            description=description,
            type=type,
            seller=seller,
            goods_name=goods_name,
            campus=campus,
            price=price,
            image=image,
            wx=wx,
        )
    else:
        conn = mysql_connect()
        cursor = conn.cursor()
        sql = "select wx from users where name='{}'".format(seller)
        cursor.execute(sql)
        wx = cursor.fetchone()
        return render_template(
            "content.html",
            description=description,
            type=type,
            seller=seller,
            goods_name=goods_name,
            campus=campus,
            price=price,
            wx=wx,
        )

# find back pw
@app.route("/other", methods=["POST", "GET"])
def other():
    return "请再注册一个账号吧……自己记好密码哈~~我没钱发信息验证身份为您找回密码"
# some small blocks
@app.route("/books", methods=["POST", "GET"])
def books():
    return render_template("books.html")
@app.route("/dailyexpense", methods=["POST", "GET"])
def dailyexpense():
    return render_template("dailyexpense.html")
@app.route("/food", methods=["POST", "GET"])
def food():
    return render_template("food.html")
@app.route("/sport", methods=["POST", "GET"])
def sport():
    return render_template("sport.html")
@app.route("/transport", methods=["POST", "GET"])
def transport():
    return render_template("transport.html")
@app.route("/electronic", methods=["POST", "GET"])
def electronic():
    return render_template("electronic.html")



if __name__ == "__main__":
    app.run(port=8080, debug=True)

'''
app.url_map.bind_to_environ().match()
app.view_functions()
app.url_map.bind()
class Flask:
    def __init__(self, import_name):
        self.import_name = import_name
        self.url_map = Map()

    def dispatch_request(self):
        url = request.url
        endpoint, values = self.url_map.bind_to_environ(request.environ).match()
        view_func = self.view_functions[endpoint]
        return view_func(**values)

    def run(self):
        while True:
            request = self.get_request()
            response = self.dispatch_request(request)
            self.send_response(response)
'''