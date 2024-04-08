import requests
from bs4 import BeautifulSoup

fields = {
    "计算机体系结构/并行与分布计算/存储系统": "https://www.ccf.org.cn/Academic_Evaluation/ARCH_DCP_SS/",
    "计算机网络": "https://www.ccf.org.cn/Academic_Evaluation/CN/",
    "网络与信息安全": "https://www.ccf.org.cn/Academic_Evaluation/NIS/",
    "软件工程/系统软件/程序设计语言": "https://www.ccf.org.cn/Academic_Evaluation/TCSE_SS_PDL/",
    "数据库/数据挖掘/内容检索": "https://www.ccf.org.cn/Academic_Evaluation/DM_CS/",
    "计算机科学理论": "https://www.ccf.org.cn/Academic_Evaluation/TCS/",
    "计算机图形学与多媒体": "https://www.ccf.org.cn/Academic_Evaluation/CGAndMT/",
    "人工智能": "https://www.ccf.org.cn/Academic_Evaluation/AI/",
    "人机交互与普适计算": "https://www.ccf.org.cn/Academic_Evaluation/HCIAndPC/",
    "交叉/综合/新兴": "https://www.ccf.org.cn/Academic_Evaluation/Cross_Compre_Emerging/",
}


# Helper function to extract information from a list item
def extract_info(li):
    divs = li.find_all("div")
    if not divs or len(divs) < 5:
        return {}  # Skip the header or invalid entries
    return {
        "序号": divs[0].text.strip(),
        "名称": divs[1].text.strip(),
        "全称": divs[2].text.strip().replace("\n", " "),
        "出版社": divs[3].text.strip(),
        "地址": divs[4].a["href"],
    }


if __name__ == "__main__":
    data = {}

    for field, url in fields.items():
        response = requests.get(url)
        response.encoding = "utf-8"
        html_content = response.text
        soup = BeautifulSoup(html_content, "html.parser")
        field_data = {
            "刊物": {"A": [], "B": [], "C": []},
            "会议": {"A": [], "B": [], "C": []},
        }

        for category in field_data.keys():
            section = soup.find("h4", string=f"中国计算机学会推荐国际学术{category}")
            for rank in field_data[category].keys():
                rank_title = section.find_next_sibling("h3", string=f"{rank}类")
                ul = rank_title.find_next_sibling("ul")
                for li in ul.find_all("li")[1:]:  # Skip the header
                    field_data[category][rank].append(extract_info(li))

        data[field] = field_data

    markdown = "> 数据来源：[CCF推荐国际学术会议和期刊目录](https://www.ccf.org.cn/Academic_Evaluation/By_category/)  \n"
    markdown += "> 期刊分区查询网站：[LetPub](https://www.letpub.com.cn/index.php?page=journalapp)\n\n"

    for field, field_data in data.items():
        markdown += f"## {field}\n\n"
        for category, category_data in field_data.items():
            markdown += f"### {field}-{category}\n\n"
            markdown += "|评级|序号|名称|全称|出版社|\n"
            markdown += "|:-:|:-:|:-:|:--|:-:|\n"
            for rank, items in category_data.items():
                for item in items:
                    markdown += f"|{rank}|{item['序号']}|{item['名称']}|{item['全称']}|{item['出版社']}|\n"
            markdown += "\n"

    with open("output.md", "w", encoding="utf-8") as f:
        f.write(markdown)
