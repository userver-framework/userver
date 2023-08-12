function init_all_results_button() {
  const searchBox = document.querySelector("#MSearchResultsWindow");
  const searchInput = document.querySelector("#MSearchField");

  const allResultsLink = document.createElement("a");
  allResultsLink.classList.add("all-results-link");

  allResultsLink.text = "All results";
  allResultsLink.target = "_blank";

  const searchURL = new URL("https://yandex.ru/search/");

  searchBox.appendChild(allResultsLink);

  searchInput.addEventListener("change", (event) => {
    searchURL.searchParams.set(
      "text",
      `site:userver.tech ${event.target.value}`
    );

    allResultsLink.href = searchURL;
  });
}
