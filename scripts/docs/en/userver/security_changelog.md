## Security Changelog

## Fixed in beta

### CVE-2023-26224: JSON parsing could lead to StackOverflow

Problem type: Stack-based Buffer Overflow

Description: The json parsing functionality in userver before 
[1410c86c4b86c3a6eebaa09207d677bef07a9313](https://github.com/userver-framework/userver/commit/1410c86c4b86c3a6eebaa09207d677bef07a9313) 
allows attackers to cause a stack overflow via crafted HTTP request.

Credits: [Ivan Trofimov](https://github.com/itrofimow)

### CVE-2022-28229: Hashing was vulnerable to HashDOS 

Problem type: Uncontrolled Resource Consumption

Description: The hash functionality in userver before
[d933af2eaf944b16cc9636a0c2893fed54434523](https://github.com/userver-framework/userver/commit/d933af2eaf944b16cc9636a0c2893fed54434523)
allows attackers to cause a denial of service via crafted HTTP request,
involving collisions.

Credits: [Ivan Trofimov](https://github.com/itrofimow)


### Hashing was vulnerable to HashDOS

Problem type: Uncontrolled Resource Consumption

Description: The hash functionality in userver before
[42059b6319661583b3080cab9b595d4f8ac48128](https://github.com/userver-framework/userver/commit/42059b6319661583b3080cab9b595d4f8ac48128)
allows attackers to cause a denial of service via crafted HTTP request,
involving collisions.

Credits: [Ivan Trofimov](https://github.com/itrofimow)
