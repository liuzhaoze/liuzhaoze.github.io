---
# the default layout is 'page'
icon: fas fa-address-book
order: 4
---

# 友情链接

<div class="friend-links">
  <div class="column">
    <div class="link-item">
      <img src="https://static.nyanners.moe/71eec8eb-a492-4f11-b59f-360299a7a14f.webp" alt="favicon" class="favicon"/>
      <a href="https://www.nyanners.moe/" class="link-text" target="_blank">X-Zero-L</a>
    </div>
    <div class="link-item">
      <img src="https://ftlian.top/wp-content/uploads/2023/04/header-150x150-2.jpg" alt="favicon" class="favicon"/>
      <a href="https://ftlian.top/" class="link-text" target="_blank">FTL's Blog</a>
    </div>
    <div class="link-item">
      <img src="https://shuhongdai.github.io/assets/img/favicon.png?6d57c5bac70ef6fae4bd96883a4eb4da" alt="favicon" class="favicon"/>
      <a href="https://shuhongdai.github.io/" class="link-text" target="_blank">Shuhong Dai</a>
    </div>
  </div>
  <div class="column">
    <div class="link-item">
      <img src="https://maxng.cc/favicon.ico" alt="favicon" class="favicon"/>
      <a href="https://maxng.cc/" class="link-text" target="_blank">Max的技术栈</a>
    </div>
    <div class="link-item">
      <img src="https://catcat.blog/wp-content/uploads/2023/12/cropped-photo_2023-11-07_02-57-03-192x192.jpg" alt="favicon" class="favicon"/>
      <a href="https://catcat.blog/" class="link-text" target="_blank">猫猫博客</a>
    </div>
  </div>
  <div class="column">
    <div class="link-item">
      <img src="https://xinalin.com/wp-content/uploads/2023/03/cropped-faviconV2-32x32.png" alt="favicon" class="favicon"/>
      <a href="https://xinalin.com/" class="link-text" target="_blank">雪林荧光</a>
    </div>
    <div class="link-item">
      <img src="https://c26h52.github.io/favicon.png" alt="favicon" class="favicon"/>
      <a href="https://c26h52.github.io/" class="link-text" target="_blank">C26H52</a>
    </div>
  </div>
</div>

<style>
/* 添加一些 CSS 来布局 */
.friend-links {
  display: flex;
  justify-content: space-between;
}
.column {
  width: 30%; /* 每一栏占据大约三分之一的宽度 */
  margin-right: 5%; /* 栏与栏之间的间隔 */
}
.column:last-child {
  margin-right: 0; /* 最后一栏不需要右边距 */
}
.link-item {
  display: flex;
  align-items: center;
  margin-bottom: 10px; /* 链接与链接之间的间隔 */
}
.favicon {
  width: 32px; /* favicon图标的大小 */
  height: 32px;
  margin-right: 10px; /* 图标和文字之间的间隔 */
}
.link-text {
  text-decoration: none;
  color: black; /* 链接文字颜色 */
}
@media(prefers-color-scheme: light) {
  html:not([data-mode]) .link-text, html[data-mode=light] .link-text {
    color: black;
  }
  html[data-mode=dark] .link-text {
    color: white;
  }
}
@media(prefers-color-scheme: dark) {
  html:not([data-mode]) .link-text, html[data-mode=dark] .link-text {
    color: white;
  }
  html[data-mode=dark] .link-text {
    color: black;
  }
}
</style>

> 欢迎向我提交 [Issue](https://github.com/liuzhaoze/liuzhaoze.github.io/issues) 交换友情链接
{: .prompt-tip }
