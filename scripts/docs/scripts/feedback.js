class PageFeedback extends HTMLElement {
  #likeCheckbox = this.#makeFeedbackCheckbox({
    className: "like-checkbox",
    title: "Helpful",
  });
  #dislikeCheckbox = this.#makeFeedbackCheckbox({
    className: "dislike-checkbox",
    title: "Not helpful",
  });
  #popup = document.createElement("page-feedback-popup");
  #docContentElement = document.getElementById("doc-content");
  #contentsElement = document.querySelector(".contents");
  #resizeObserver = new ResizeObserver(this.#updatePosition.bind(this));

  static init() {
    const toc = document.querySelector(".toc");

    if (toc === null) {
      return;
    } else {
      toc.parentNode.insertBefore(document.createElement("page-feedback"), toc);
    }
  }

  constructor() {
    super();
    this.#addEventListeners();
  }

  #addEventListeners() {
    this.#likeCheckbox.addEventListener(
      "change",
      this.#getCheckboxChangeCallback({
        checkbox: this.#likeCheckbox,
        oppositeCheckbox: this.#dislikeCheckbox,
        modalOffset: -152,
        title: "Thank you!",
      })
    );
    this.#dislikeCheckbox.addEventListener(
      "change",
      this.#getCheckboxChangeCallback({
        checkbox: this.#dislikeCheckbox,
        oppositeCheckbox: this.#likeCheckbox,
        modalOffset: -125,
        title: "Tell us what's wrong",
      })
    );
  }

  #getCheckboxChangeCallback({
    checkbox,
    oppositeCheckbox,
    modalOffset,
    title,
  }) {
    return () => {
      if (checkbox.checked) {
        this.#popup.open({ modalOffset, title });
        oppositeCheckbox.checked = false;
      } else if (!oppositeCheckbox.checked) {
        this.#popup.close();
      }
    };
  }

  connectedCallback() {
    this.appendChild(this.#likeCheckbox);
    this.appendChild(this.#dislikeCheckbox);
    this.appendChild(this.#popup);

    this.#resizeObserver.observe(document.body);
  }

  disconnectedCallback() {
    this.#resizeObserver.unobserve(document.body);
  }

  #makeFeedbackCheckbox({ className, title }) {
    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.title = title;
    checkbox.className = className;
    return checkbox;
  }

  #updatePosition() {
    const marginRight = parseFloat(
      window.getComputedStyle(this.#contentsElement).marginRight
    );
    const paddingRight = parseFloat(
      window.getComputedStyle(this.#contentsElement).paddingRight
    );
    const scrollbarWidth = this.#docContentElement.offsetWidth - this.#docContentElement.clientWidth;
    this.style.right = `${marginRight + paddingRight + scrollbarWidth}px`;
  }
}

class PageFeedbackPopup extends HTMLElement {
  static #inactiveCloseTimeMilliseconds = 5000;

  #title = document.createElement("h6");
  #paragraph = this.#makeParagraph(
    "Your opinion will help to improve our service"
  );
  #link = this.#makeFeedbackLink(
    "https://forms.yandex.ru/u/667d482fe010db2f53e00edf/"
  );

  #currentFadeOutAnimation = null;

  #inactivityCloseTimerID = null;
  #clickOutsidePopupHandlerCallback = null;

  get #isOpen() {
    return this.style.opacity >= 1;
  }

  constructor() {
    super();
    this.#addEventListeners();
  }

  #addEventListeners() {
    this.addEventListener(
      "mouseenter",
      this.#removeInactivityCloseTimer.bind(this)
    );
    this.addEventListener(
      "mouseleave",
      this.#startInactivityCloseTimer.bind(this)
    );
  }

  connectedCallback() {
    this.appendChild(this.#title);
    this.appendChild(this.#paragraph);
    this.appendChild(this.#link);
  }

  async open({ modalOffset, title }) {
    if (this.#isOpen) {
      await this.close();
    }

    this.#title.textContent = title;
    this.style.left = `${modalOffset}px`;
    await this.#fadeIn();
    this.#startInactivityCloseTimer();
    this.#addClickOutsidePopupCloseHandler();
  }

  #startInactivityCloseTimer() {
    if (this.#inactivityCloseTimerID === null) {
      this.#inactivityCloseTimerID = setInterval(
        this.close.bind(this),
        PageFeedbackPopup.#inactiveCloseTimeMilliseconds
      );
    } else {
      return;
    }
  }

  #addClickOutsidePopupCloseHandler() {
    this.#clickOutsidePopupHandlerCallback = (event) => {
      const elementsUnderCursor = document.elementsFromPoint(
        event.clientX,
        event.clientY
      );
      const isPopupUnderCursor = elementsUnderCursor.some(
        (element) => element === this
      );
      if (!isPopupUnderCursor) {
        this.close();
      }
    };

    document.addEventListener("click", this.#clickOutsidePopupHandlerCallback);
  }

  async #fadeIn() {
    return new Promise((resolve) => {
      let opacity = 0;
      this.style.zIndex = 10;
      const timer = setInterval(() => {
        if (opacity < 1) {
          opacity += 0.05;
          this.style.opacity = opacity;
        } else {
          this.style.opacity = 1;
          clearInterval(timer);
          resolve();
        }
      }, 6);
    });
  }

  async close() {
    this.#removeInactivityCloseTimer();
    this.#removeClickOutsidePopupCloseHandler();
    await this.#fadeOut();
  }

  #removeInactivityCloseTimer() {
    clearInterval(this.#inactivityCloseTimerID);
    this.#inactivityCloseTimerID = null;
  }

  #removeClickOutsidePopupCloseHandler() {
    document.removeEventListener(
      "click",
      this.#clickOutsidePopupHandlerCallback
    );
    this.#clickOutsidePopupHandlerCallback = null;
  }

  async #fadeOut() {
    if (this.#currentFadeOutAnimation === null) {
      return this.#startFadeOutAnimation();
    } else {
      return this.#currentFadeOutAnimation;
    }
  }

  #startFadeOutAnimation() {
    this.#currentFadeOutAnimation = new Promise((resolve) => {
      let opacity = this.style.opacity;
      const timer = setInterval(() => {
        if (opacity > 0) {
          opacity -= 0.05;
          this.style.opacity = opacity;
        } else {
          this.style.zIndex = 0;
          this.style.opacity = 0;
          clearInterval(timer);
          this.#currentFadeOutAnimation = null;
          resolve();
        }
      }, 6);
    });

    return this.#currentFadeOutAnimation;
  }

  #makeParagraph(text) {
    const paragraph = document.createElement("p");
    paragraph.textContent = text;
    return paragraph;
  }

  #makeFeedbackLink(href) {
    const link = document.createElement("a");
    link.href = href;
    link.target = "_blank";
    link.textContent = "Leave a feedback >";
    link.title = "Fill out the feedback form";
    return link;
  }
}

customElements.define("page-feedback", PageFeedback);
customElements.define("page-feedback-popup", PageFeedbackPopup);
